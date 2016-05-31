/* NetHack 3.6	dokick.c	$NHDT-Date: 1446955295 2015/11/08 04:01:35 $  $NHDT-Branch: master $:$NHDT-Revision: 1.104 $ */
/* Copyright (c) Izchak Miller, Mike Stephenson, Steve Linhart, 1989. */
/* NetHack may be freely redistributed.  See license for details. */

/* JNetHack Copyright */
/* (c) Issei Numata, Naoki Hamada, Shigehiro Miyashita, 1994-2000  */
/* For 3.4-, Copyright (c) SHIRAKATA Kentaro, 2002-2016            */
/* JNetHack may be freely redistributed.  See license for details. */

#include "hack.h"

#define is_bigfoot(x) ((x) == &mons[PM_SASQUATCH])
#define martial()                                 \
    (martial_bonus() || is_bigfoot(youmonst.data) \
     || (uarmf && uarmf->otyp == KICKING_BOOTS))

static NEARDATA struct rm *maploc, nowhere;
static NEARDATA const char *gate_str;

/* kickedobj (decl.c) tracks a kicked object until placed or destroyed */

extern boolean notonhead; /* for long worms */

STATIC_DCL void FDECL(kickdmg, (struct monst *, BOOLEAN_P));
STATIC_DCL boolean FDECL(maybe_kick_monster, (struct monst *,
                                              XCHAR_P, XCHAR_P));
STATIC_DCL void FDECL(kick_monster, (struct monst *, XCHAR_P, XCHAR_P));
STATIC_DCL int FDECL(kick_object, (XCHAR_P, XCHAR_P));
STATIC_DCL int FDECL(really_kick_object, (XCHAR_P, XCHAR_P));
STATIC_DCL char *FDECL(kickstr, (char *));
STATIC_DCL void FDECL(otransit_msg, (struct obj *, BOOLEAN_P, long));
STATIC_DCL void FDECL(drop_to, (coord *, SCHAR_P));

/*JP
static const char kick_passes_thru[] = "kick passes harmlessly through";
*/
static const char kick_passes_thru[] = "蹴りはダメージを与えずにすり抜けた";

STATIC_OVL void
kickdmg(mon, clumsy)
register struct monst *mon;
register boolean clumsy;
{
    register int mdx, mdy;
    register int dmg = (ACURRSTR + ACURR(A_DEX) + ACURR(A_CON)) / 15;
    int kick_skill = P_NONE;
    int blessed_foot_damage = 0;
    boolean trapkilled = FALSE;

    if (uarmf && uarmf->otyp == KICKING_BOOTS)
        dmg += 5;

    /* excessive wt affects dex, so it affects dmg */
    if (clumsy)
        dmg /= 2;

    /* kicking a dragon or an elephant will not harm it */
    if (thick_skinned(mon->data))
        dmg = 0;

    /* attacking a shade is useless */
    if (mon->data == &mons[PM_SHADE])
        dmg = 0;

    if ((is_undead(mon->data) || is_demon(mon->data) || is_vampshifter(mon))
        && uarmf && uarmf->blessed)
        blessed_foot_damage = 1;

    if (mon->data == &mons[PM_SHADE] && !blessed_foot_damage) {
/*JP
        pline_The("%s.", kick_passes_thru);
*/
        pline_The("%s．", kick_passes_thru);
        /* doesn't exercise skill or abuse alignment or frighten pet,
           and shades have no passive counterattack */
        return;
    }

    if (mon->m_ap_type)
        seemimic(mon);

    check_caitiff(mon);

    /* squeeze some guilt feelings... */
    if (mon->mtame) {
        abuse_dog(mon);
        if (mon->mtame)
            monflee(mon, (dmg ? rnd(dmg) : 1), FALSE, FALSE);
        else
            mon->mflee = 0;
    }

    if (dmg > 0) {
        /* convert potential damage to actual damage */
        dmg = rnd(dmg);
        if (martial()) {
            if (dmg > 1)
                kick_skill = P_MARTIAL_ARTS;
            dmg += rn2(ACURR(A_DEX) / 2 + 1);
        }
        /* a good kick exercises your dex */
        exercise(A_DEX, TRUE);
    }
    if (blessed_foot_damage)
        dmg += rnd(4);
    if (uarmf)
        dmg += uarmf->spe;
    dmg += u.udaminc; /* add ring(s) of increase damage */
    if (dmg > 0)
        mon->mhp -= dmg;
    if (mon->mhp > 0 && martial() && !bigmonst(mon->data) && !rn2(3)
        && mon->mcanmove && mon != u.ustuck && !mon->mtrapped) {
        /* see if the monster has a place to move into */
        mdx = mon->mx + u.dx;
        mdy = mon->my + u.dy;
        if (goodpos(mdx, mdy, mon, 0)) {
/*JP
            pline("%s reels from the blow.", Monnam(mon));
*/
            pline("%sは強打されよろめいた．", Monnam(mon));
            if (m_in_out_region(mon, mdx, mdy)) {
                remove_monster(mon->mx, mon->my);
                newsym(mon->mx, mon->my);
                place_monster(mon, mdx, mdy);
                newsym(mon->mx, mon->my);
                set_apparxy(mon);
                if (mintrap(mon) == 2)
                    trapkilled = TRUE;
            }
        }
    }

    (void) passive(mon, TRUE, mon->mhp > 0, AT_KICK, FALSE);
    if (mon->mhp <= 0 && !trapkilled)
        killed(mon);

    /* may bring up a dialog, so put this after all messages */
    if (kick_skill != P_NONE) /* exercise proficiency */
        use_skill(kick_skill, 1);
}

STATIC_OVL boolean
maybe_kick_monster(mon, x, y)
struct monst *mon;
xchar x, y;
{
    if (mon) {
        boolean save_forcefight = context.forcefight;

        bhitpos.x = x;
        bhitpos.y = y;
        if (!mon->mpeaceful || !canspotmon(mon))
            context.forcefight = TRUE; /* attack even if invisible */
        /* kicking might be halted by discovery of hidden monster,
           by player declining to attack peaceful monster,
           or by passing out due to encumbrance */
        if (attack_checks(mon, (struct obj *) 0) || overexertion())
            mon = 0; /* don't kick after all */
        context.forcefight = save_forcefight;
    }
    return (boolean) (mon != 0);
}

STATIC_OVL void
kick_monster(mon, x, y)
struct monst *mon;
xchar x, y;
{
    boolean clumsy = FALSE;
    int i, j;

    /* anger target even if wild miss will occur */
    setmangry(mon);

    if (Levitation && !rn2(3) && verysmall(mon->data)
        && !is_flyer(mon->data)) {
/*JP
        pline("Floating in the air, you miss wildly!");
*/
        pline("空中に浮いているので，大きく外した！");
        exercise(A_DEX, FALSE);
        (void) passive(mon, FALSE, 1, AT_KICK, FALSE);
        return;
    }

    /* reveal hidden target even if kick ends up missing (note: being
       hidden doesn't affect chance to hit so neither does this reveal) */
    if (mon->mundetected
        || (mon->m_ap_type && mon->m_ap_type != M_AP_MONSTER)) {
        if (mon->m_ap_type)
            seemimic(mon);
        mon->mundetected = 0;
        if (!canspotmon(mon))
            map_invisible(x, y);
        else
            newsym(x, y);
#if 0 /*JP*/
        There("is %s here.",
              canspotmon(mon) ? a_monnam(mon) : "something hidden");
#else
        There("%sがいる．",
              canspotmon(mon) ? a_monnam(mon) : "何か隠れているもの");
#endif
    }

    /* Kick attacks by kicking monsters are normal attacks, not special.
     * This is almost always worthless, since you can either take one turn
     * and do all your kicks, or else take one turn and attack the monster
     * normally, getting all your attacks _including_ all your kicks.
     * If you have >1 kick attack, you get all of them.
     */
    if (Upolyd && attacktype(youmonst.data, AT_KICK)) {
        struct attack *uattk;
        int sum, kickdieroll, armorpenalty,
            attknum = 0,
            tmp = find_roll_to_hit(mon, AT_KICK, (struct obj *) 0, &attknum,
                                   &armorpenalty);

        for (i = 0; i < NATTK; i++) {
            /* first of two kicks might have provoked counterattack
               that has incapacitated the hero (ie, floating eye) */
            if (multi < 0)
                break;

            uattk = &youmonst.data->mattk[i];
            /* we only care about kicking attacks here */
            if (uattk->aatyp != AT_KICK)
                continue;

            if (mon->data == &mons[PM_SHADE] && (!uarmf || !uarmf->blessed)) {
                /* doesn't matter whether it would have hit or missed,
                   and shades have no passive counterattack */
/*JP
                Your("%s %s.", kick_passes_thru, mon_nam(mon));
*/
                You("%sを蹴ったが，%s．", mon_nam(mon), kick_passes_thru);
                break; /* skip any additional kicks */
            } else if (tmp > (kickdieroll = rnd(20))) {
/*JP
                You("kick %s.", mon_nam(mon));
*/
                You("%sを蹴った．", mon_nam(mon));
                sum = damageum(mon, uattk);
                (void) passive(mon, (boolean) (sum > 0), (sum != 2), AT_KICK,
                               FALSE);
                if (sum == 2)
                    break; /* Defender died */
            } else {
                missum(mon, uattk, (tmp + armorpenalty > kickdieroll));
                (void) passive(mon, FALSE, 1, AT_KICK, FALSE);
            }
        }
        return;
    }

    i = -inv_weight();
    j = weight_cap();

    if (i < (j * 3) / 10) {
        if (!rn2((i < j / 10) ? 2 : (i < j / 5) ? 3 : 4)) {
            if (martial() && !rn2(2))
                goto doit;
/*JP
            Your("clumsy kick does no damage.");
*/
            Your("不器用な蹴りでダメージを与えられなかった．");
            (void) passive(mon, FALSE, 1, AT_KICK, FALSE);
            return;
        }
        if (i < j / 10)
            clumsy = TRUE;
        else if (!rn2((i < j / 5) ? 2 : 3))
            clumsy = TRUE;
    }

    if (Fumbling)
        clumsy = TRUE;

    else if (uarm && objects[uarm->otyp].oc_bulky && ACURR(A_DEX) < rnd(25))
        clumsy = TRUE;
doit:
/*JP
    You("kick %s.", mon_nam(mon));
*/
    You("%sを蹴った．", mon_nam(mon));
    if (!rn2(clumsy ? 3 : 4) && (clumsy || !bigmonst(mon->data))
        && mon->mcansee && !mon->mtrapped && !thick_skinned(mon->data)
        && mon->data->mlet != S_EEL && haseyes(mon->data) && mon->mcanmove
        && !mon->mstun && !mon->mconf && !mon->msleeping
        && mon->data->mmove >= 12) {
        if (!nohands(mon->data) && !rn2(martial() ? 5 : 3)) {
#if 0 /*JP:T*/
            pline("%s blocks your %skick.", Monnam(mon),
                  clumsy ? "clumsy " : "");
#else
            pline("%sはあなたの%s蹴りを防いだ．", Monnam(mon),
                  clumsy ? "不器用な" : "");
#endif
            (void) passive(mon, FALSE, 1, AT_KICK, FALSE);
            return;
        } else {
            maybe_mnexto(mon);
            if (mon->mx != x || mon->my != y) {
                if (glyph_is_invisible(levl[x][y].glyph)) {
                    unmap_object(x, y);
                    newsym(x, y);
                }
#if 0 /*JP*/
                pline("%s %s, %s evading your %skick.", Monnam(mon),
                      (!level.flags.noteleport && can_teleport(mon->data))
                          ? "teleports"
                          : is_floater(mon->data)
                                ? "floats"
                                : is_flyer(mon->data) ? "swoops"
                                                      : (nolimbs(mon->data)
                                                         || slithy(mon->data))
                                                            ? "slides"
                                                            : "jumps",
                      clumsy ? "easily" : "nimbly", clumsy ? "clumsy " : "");
#else
                pline("%sは%s，%sあなたの%s蹴りをたくみに避けた．", Monnam(mon),
                      (!level.flags.noteleport && can_teleport(mon->data))
                          ? "瞬間移動し"
                          : is_floater(mon->data) ? "浮き"
                                : is_flyer(mon->data) ? "はばたき"
                                                      : (nolimbs(mon->data)
                                                         || slithy(mon->data))
                                                            ? "横に滑り"
                                                            : "跳ね",
                      clumsy ? "楽々と" : "素早く", clumsy ? "不器用な" : "");
#endif
                (void) passive(mon, FALSE, 1, AT_KICK, FALSE);
                return;
            }
        }
    }
    kickdmg(mon, clumsy);
}

/*
 *  Return TRUE if caught (the gold taken care of), FALSE otherwise.
 *  The gold object is *not* attached to the fobj chain!
 */
boolean
ghitm(mtmp, gold)
register struct monst *mtmp;
register struct obj *gold;
{
    boolean msg_given = FALSE;

    if (!likes_gold(mtmp->data) && !mtmp->isshk && !mtmp->ispriest
        && !mtmp->isgd && !is_mercenary(mtmp->data)) {
        wakeup(mtmp);
    } else if (!mtmp->mcanmove) {
        /* too light to do real damage */
        if (canseemon(mtmp)) {
#if 0 /*JP:T*/
            pline_The("%s harmlessly %s %s.", xname(gold),
                      otense(gold, "hit"), mon_nam(mtmp));
#else
            pline("%sは%sに命中した．", xname(gold),
                  mon_nam(mtmp));
#endif
            msg_given = TRUE;
        }
    } else {
        long umoney, value = gold->quan * objects[gold->otyp].oc_cost;

        mtmp->msleeping = 0;
        finish_meating(mtmp);
        if (!mtmp->isgd && !rn2(4)) /* not always pleasing */
            setmangry(mtmp);
        /* greedy monsters catch gold */
        if (cansee(mtmp->mx, mtmp->my))
/*JP
            pline("%s catches the gold.", Monnam(mtmp));
*/
            pline("%sは金貨をキャッチした．", Monnam(mtmp));
        (void) mpickobj(mtmp, gold);
        gold = (struct obj *) 0; /* obj has been freed */
        if (mtmp->isshk) {
            long robbed = ESHK(mtmp)->robbed;

            if (robbed) {
                robbed -= value;
                if (robbed < 0L)
                    robbed = 0L;
#if 0 /*JP*/
                pline_The("amount %scovers %s recent losses.",
                          !robbed ? "" : "partially ", mhis(mtmp));
#else
                pline("%s%s損失を補填するのに使われた．",
                      !robbed ? "" : "金の一部は", mhis(mtmp));
#endif
                ESHK(mtmp)->robbed = robbed;
                if (!robbed)
                    make_happy_shk(mtmp, FALSE);
            } else {
                if (mtmp->mpeaceful) {
                    ESHK(mtmp)->credit += value;
#if 0 /*JP*/
                    You("have %ld %s in credit.", ESHK(mtmp)->credit,
                        currency(ESHK(mtmp)->credit));
#else
                    You("%ld%sを貸しにした．", ESHK(mtmp)->credit,
                        currency(ESHK(mtmp)->credit));
#endif
                } else
/*JP
                    verbalize("Thanks, scum!");
*/
                    verbalize("ありがとよ！くそったれ！");
            }
        } else if (mtmp->ispriest) {
            if (mtmp->mpeaceful)
/*JP
                verbalize("Thank you for your contribution.");
*/
                verbalize("寄付に感謝します．");
            else
/*JP
                verbalize("Thanks, scum!");
*/
                verbalize("ありがとよ！くそったれ！");
        } else if (mtmp->isgd) {
            umoney = money_cnt(invent);
            /* Some of these are iffy, because a hostile guard
               won't become peaceful and resume leading hero
               out of the vault.  If he did do that, player
               could try fighting, then weasle out of being
               killed by throwing his/her gold when losing. */
#if 0 /*JP*/
            verbalize(
                umoney
                    ? "Drop the rest and follow me."
                    : hidden_gold()
                          ? "You still have hidden gold.  Drop it now."
                          : mtmp->mpeaceful
                                ? "I'll take care of that; please move along."
                                : "I'll take that; now get moving.");
#else
            verbalize(
                umoney
                    ? "残りを置いてついてきなさい．"
                    : hidden_gold()
                          ? "まだ金を隠しているな．置きなさい．"
                          : mtmp->mpeaceful
                                ? "それは私が拾っておきますからついてきてください．"
                                : "それは拾っておく．来なさい．");
#endif
        } else if (is_mercenary(mtmp->data)) {
            long goldreqd = 0L;

            if (rn2(3)) {
                if (mtmp->data == &mons[PM_SOLDIER])
                    goldreqd = 100L;
                else if (mtmp->data == &mons[PM_SERGEANT])
                    goldreqd = 250L;
                else if (mtmp->data == &mons[PM_LIEUTENANT])
                    goldreqd = 500L;
                else if (mtmp->data == &mons[PM_CAPTAIN])
                    goldreqd = 750L;

                if (goldreqd) {
                    umoney = money_cnt(invent);
                    if (value
                        > goldreqd
                              + (umoney + u.ulevel * rn2(5)) / ACURR(A_CHA))
                        mtmp->mpeaceful = TRUE;
                }
            }
            if (mtmp->mpeaceful)
/*JP
                verbalize("That should do.  Now beat it!");
*/
                verbalize("なんだい？これは？");
            else
/*JP
                verbalize("That's not enough, coward!");
*/
                verbalize("そんなもので済むか，卑怯者！");
        }
        return TRUE;
    }

    if (!msg_given)
        miss(xname(gold), mtmp);
    return FALSE;
}

/* container is kicked, dropped, thrown or otherwise impacted by player.
 * Assumes container is on floor.  Checks contents for possible damage. */
void
container_impact_dmg(obj, x, y)
struct obj *obj;
xchar x, y; /* coordinates where object was before the impact, not after */
{
    struct monst *shkp;
    struct obj *otmp, *otmp2;
    long loss = 0L;
    boolean costly, insider, frominv;

    /* only consider normal containers */
    if (!Is_container(obj) || !Has_contents(obj) || Is_mbag(obj))
        return;

    costly = ((shkp = shop_keeper(*in_rooms(x, y, SHOPBASE)))
              && costly_spot(x, y));
    insider = (*u.ushops && inside_shop(u.ux, u.uy)
               && *in_rooms(x, y, SHOPBASE) == *u.ushops);
    /* if dropped or thrown, shop ownership flags are set on this obj */
    frominv = (obj != kickedobj);

    for (otmp = obj->cobj; otmp; otmp = otmp2) {
        const char *result = (char *) 0;

        otmp2 = otmp->nobj;
        if (objects[otmp->otyp].oc_material == GLASS
            && otmp->oclass != GEM_CLASS && !obj_resists(otmp, 33, 100)) {
/*JP
            result = "shatter";
*/
            result = "ガチャン";
        } else if (otmp->otyp == EGG && !rn2(3)) {
/*JP
            result = "cracking";
*/
            result = "グシャッ";
        }
        if (result) {
            if (otmp->otyp == MIRROR)
                change_luck(-2);

            /* eggs laid by you.  penalty is -1 per egg, max 5,
             * but it's always exactly 1 that breaks */
            if (otmp->otyp == EGG && otmp->spe && otmp->corpsenm >= LOW_PM)
                change_luck(-1);
/*JP
            You_hear("a muffled %s.", result);
*/
            You_hear("こもった%sという音を聞いた．", result);
            if (costly) {
                if (frominv && !otmp->unpaid)
                    otmp->no_charge = 1;
                loss +=
                    stolen_value(otmp, x, y, (boolean) shkp->mpeaceful, TRUE);
            }
            if (otmp->quan > 1L) {
                useup(otmp);
            } else {
                obj_extract_self(otmp);
                obfree(otmp, (struct obj *) 0);
            }
            /* contents of this container are no longer known */
            obj->cknown = 0;
        }
    }
    if (costly && loss) {
        if (!insider) {
/*JP
            You("caused %ld %s worth of damage!", loss, currency(loss));
*/
            You("%ld%s分の損害をひきおこした！", loss, currency(loss));
            make_angry_shk(shkp, x, y);
        } else {
#if 0 /*JP*/
            You("owe %s %ld %s for objects destroyed.", mon_nam(shkp), loss,
                currency(loss));
#else
            You("器物破損で%sに%ld%sの借りをつくった．", mon_nam(shkp), loss,
                currency(loss));
#endif
        }
    }
}

/* jacket around really_kick_object */
STATIC_OVL int
kick_object(x, y)
xchar x, y;
{
    int res = 0;

    /* if a pile, the "top" object gets kicked */
    kickedobj = level.objects[x][y];
    if (kickedobj) {
        /* kick object; if doing is fatal, done() will clean up kickedobj */
        res = really_kick_object(x, y);
        kickedobj = (struct obj *) 0;
    }
    return res;
}

/* guts of kick_object */
STATIC_OVL int
really_kick_object(x, y)
xchar x, y;
{
    int range;
    struct monst *mon, *shkp = 0;
    struct trap *trap;
    char bhitroom;
    boolean costly, isgold, slide = FALSE;

    /* kickedobj should always be set due to conditions of call */
    if (!kickedobj || kickedobj->otyp == BOULDER || kickedobj == uball
        || kickedobj == uchain)
        return 0;

    if ((trap = t_at(x, y)) != 0
        && (((trap->ttyp == PIT || trap->ttyp == SPIKED_PIT) && !Passes_walls)
            || trap->ttyp == WEB)) {
        if (!trap->tseen)
            find_trap(trap);
#if 0 /*JP*/
        You_cant("kick %s that's in a %s!", something,
                 Hallucination ? "tizzy" : (trap->ttyp == WEB) ? "web"
                                                               : "pit");
#else
        You("%sでは%sを蹴ることができない！",
            Hallucination ? "混乱した状態"
                          : trap->ttyp == WEB
                             ? "くもの巣の中" : "落し穴の中",
            something);
#endif
        return 1;
    }

    if (Fumbling && !rn2(3)) {
/*JP
        Your("clumsy kick missed.");
*/
        Your("不器用な蹴りは外れた．");
        return 1;
    }

    if (!uarmf && kickedobj->otyp == CORPSE
        && touch_petrifies(&mons[kickedobj->corpsenm]) && !Stone_resistance) {
#if 0 /*JP*/
        You("kick %s with your bare %s.",
            corpse_xname(kickedobj, (const char *) 0, CXN_PFX_THE),
            makeplural(body_part(FOOT)));
#else
        You("素%sで%sを蹴った．",
            body_part(FOOT),
            corpse_xname(kickedobj, (const char *) 0, CXN_PFX_THE));
#endif
        if (poly_when_stoned(youmonst.data) && polymon(PM_STONE_GOLEM)) {
            ; /* hero has been transformed but kick continues */
        } else {
            /* normalize body shape here; foot, not body_part(FOOT) */
/*JP
            Sprintf(killer.name, "kicking %s barefoot",
*/
            Sprintf(killer.name, "靴無しで%sを蹴って",
                    killer_xname(kickedobj));
            instapetrify(killer.name);
        }
    }

    /* range < 2 means the object will not move.  */
    /* maybe dexterity should also figure here.   */
    range = (int) ((ACURRSTR) / 2 - kickedobj->owt / 40);

    if (martial())
        range += rnd(3);

    if (is_pool(x, y)) {
        /* you're in the water too; significantly reduce range */
        range = range / 3 + 1; /* {1,2}=>1, {3,4,5}=>2, {6,7,8}=>3 */
    } else if (Is_airlevel(&u.uz) || Is_waterlevel(&u.uz)) {
        /* you're in air, since is_pool did not match */
        range += rnd(3);
    } else {
        if (is_ice(x, y))
            range += rnd(3), slide = TRUE;
        if (kickedobj->greased)
            range += rnd(3), slide = TRUE;
    }

    /* Mjollnir is magically too heavy to kick */
    if (kickedobj->oartifact == ART_MJOLLNIR)
        range = 1;

    /* see if the object has a place to move into */
    if (!ZAP_POS(levl[x + u.dx][y + u.dy].typ)
        || closed_door(x + u.dx, y + u.dy))
        range = 1;

    costly = (!(kickedobj->no_charge && !Has_contents(kickedobj))
              && (shkp = shop_keeper(*in_rooms(x, y, SHOPBASE))) != 0
              && costly_spot(x, y));
    isgold = (kickedobj->oclass == COIN_CLASS);

    if (IS_ROCK(levl[x][y].typ) || closed_door(x, y)) {
        if ((!martial() && rn2(20) > ACURR(A_DEX))
            || IS_ROCK(levl[u.ux][u.uy].typ) || closed_door(u.ux, u.uy)) {
            if (Blind)
/*JP
                pline("It doesn't come loose.");
*/
                pline("びくともしない．");
            else
#if 0 /*JP*/
                pline("%s %sn't come loose.",
                      The(distant_name(kickedobj, xname)),
                      otense(kickedobj, "do"));
#else
                pline("%sはびくともしない．",
                      distant_name(kickedobj, xname));
#endif
            return (!rn2(3) || martial());
        }
        if (Blind)
/*JP
            pline("It comes loose.");
*/
            pline("何かが緩んでとれた．");
        else
#if 0 /*JP*/
            pline("%s %s loose.", The(distant_name(kickedobj, xname)),
                  otense(kickedobj, "come"));
#else
            pline("%sは緩んでとれた．", distant_name(kickedobj, xname));
#endif
        obj_extract_self(kickedobj);
        newsym(x, y);
        if (costly && (!costly_spot(u.ux, u.uy)
                       || !index(u.urooms, *in_rooms(x, y, SHOPBASE))))
            addtobill(kickedobj, FALSE, FALSE, FALSE);
/*JP
        if (!flooreffects(kickedobj, u.ux, u.uy, "fall")) {
*/
        if (!flooreffects(kickedobj, u.ux, u.uy, "落ちる")) {
            place_object(kickedobj, u.ux, u.uy);
            stackobj(kickedobj);
            newsym(u.ux, u.uy);
        }
        return 1;
    }

    /* a box gets a chance of breaking open here */
    if (Is_box(kickedobj)) {
        boolean otrp = kickedobj->otrapped;

        if (range < 2)
/*JP
            pline("THUD!");
*/
            pline("ガン！");
        container_impact_dmg(kickedobj, x, y);
        if (kickedobj->olocked) {
            if (!rn2(5) || (martial() && !rn2(2))) {
/*JP
                You("break open the lock!");
*/
                You("鍵を壊し開けた！");
                breakchestlock(kickedobj, FALSE);
                if (otrp)
                    (void) chest_trap(kickedobj, LEG, FALSE);
                return 1;
            }
        } else {
            if (!rn2(3) || (martial() && !rn2(2))) {
/*JP
                pline_The("lid slams open, then falls shut.");
*/
                pline("蓋がばたんと開き，閉じた．");
                kickedobj->lknown = 1;
                if (otrp)
                    (void) chest_trap(kickedobj, LEG, FALSE);
                return 1;
            }
        }
        if (range < 2)
            return 1;
        /* else let it fall through to the next cases... */
    }

    /* fragile objects should not be kicked */
    if (hero_breaks(kickedobj, kickedobj->ox, kickedobj->oy, FALSE))
        return 1;

    /* too heavy to move.  range is calculated as potential distance from
     * player, so range == 2 means the object may move up to one square
     * from its current position
     */
    if (range < 2) {
        if (!Is_box(kickedobj))
/*JP
            pline("Thump!");
*/
            pline("ゴツン！");
        return (!rn2(3) || martial());
    }

    if (kickedobj->quan > 1L) {
        if (!isgold) {
            kickedobj = splitobj(kickedobj, 1L);
        } else {
            if (rn2(20)) {
#if 0 /*JP*/
                static NEARDATA const char *const flyingcoinmsg[] = {
                    "scatter the coins", "knock coins all over the place",
                    "send coins flying in all directions",
                };
#else
                static NEARDATA const char *const flyingcoinmsg[] = {
                    "金貨をまき散らした", "金貨をばらまいた",
                    "金貨をあちこちに飛ばした",
                };
#endif

/*JP
                pline("Thwwpingg!");
*/
                pline("ガシャーン！");
/*JP
                You("%s!", flyingcoinmsg[rn2(SIZE(flyingcoinmsg))]);
*/
                You("%s！", flyingcoinmsg[rn2(SIZE(flyingcoinmsg))]);
                (void) scatter(x, y, rn2(3) + 1, VIS_EFFECTS | MAY_HIT,
                               kickedobj);
                newsym(x, y);
                return 1;
            }
            if (kickedobj->quan > 300L) {
/*JP
                pline("Thump!");
*/
                pline("ゴツン！");
                return (!rn2(3) || martial());
            }
        }
    }

    if (slide && !Blind)
#if 0 /*JP*/
        pline("Whee!  %s %s across the %s.", Doname2(kickedobj),
              otense(kickedobj, "slide"), surface(x, y));
#else
        pline("ズルッ！%sは%sの上を滑った．", Doname2(kickedobj),
              surface(x,y));
#endif

    if (costly && !isgold)
        addtobill(kickedobj, FALSE, FALSE, TRUE);
    obj_extract_self(kickedobj);
    (void) snuff_candle(kickedobj);
    newsym(x, y);
    mon = bhit(u.dx, u.dy, range, KICKED_WEAPON,
               (int FDECL((*), (MONST_P, OBJ_P))) 0,
               (int FDECL((*), (OBJ_P, OBJ_P))) 0, &kickedobj);
    if (!kickedobj)
        return 1; /* object broken */

    if (mon) {
        if (mon->isshk && kickedobj->where == OBJ_MINVENT
            && kickedobj->ocarry == mon)
            return 1; /* alert shk caught it */
        notonhead = (mon->mx != bhitpos.x || mon->my != bhitpos.y);
        if (isgold ? ghitm(mon, kickedobj)      /* caught? */
                   : thitmonst(mon, kickedobj)) /* hit && used up? */
            return 1;
    }

    /* the object might have fallen down a hole;
       ship_object() will have taken care of shop billing */
    if (kickedobj->where == OBJ_MIGRATING)
        return 1;

    bhitroom = *in_rooms(bhitpos.x, bhitpos.y, SHOPBASE);
    if (costly && (!costly_spot(bhitpos.x, bhitpos.y)
                   || *in_rooms(x, y, SHOPBASE) != bhitroom)) {
        if (isgold)
            costly_gold(x, y, kickedobj->quan);
        else
            (void) stolen_value(kickedobj, x, y, (boolean) shkp->mpeaceful,
                                FALSE);
    }

/*JP
    if (flooreffects(kickedobj, bhitpos.x, bhitpos.y, "fall"))
*/
    if(flooreffects(kickedobj, bhitpos.x, bhitpos.y, "落ちる"))
        return 1;
    if (kickedobj->unpaid)
        subfrombill(kickedobj, shkp);
    place_object(kickedobj, bhitpos.x, bhitpos.y);
    stackobj(kickedobj);
    newsym(kickedobj->ox, kickedobj->oy);
    return 1;
}

/* cause of death if kicking kills kicker */
STATIC_OVL char *
kickstr(buf)
char *buf;
{
    const char *what;

    if (kickedobj)
        what = killer_xname(kickedobj);
    else if (maploc == &nowhere)
/*JP
        what = "nothing";
*/
        what = "何もないところ";
    else if (IS_DOOR(maploc->typ))
/*JP
        what = "a door";
*/
        what = "扉";
    else if (IS_TREE(maploc->typ))
/*JP
        what = "a tree";
*/
        what = "木";
    else if (IS_STWALL(maploc->typ))
/*JP
        what = "a wall";
*/
        what = "壁";
    else if (IS_ROCK(maploc->typ))
/*JP
        what = "a rock";
*/
        what = "岩";
    else if (IS_THRONE(maploc->typ))
/*JP
        what = "a throne";
*/
        what = "玉座";
    else if (IS_FOUNTAIN(maploc->typ))
/*JP
        what = "a fountain";
*/
        what = "泉";
    else if (IS_GRAVE(maploc->typ))
/*JP
        what = "a headstone";
*/
        what = "墓石";
    else if (IS_SINK(maploc->typ))
/*JP
        what = "a sink";
*/
        what = "流し台";
    else if (IS_ALTAR(maploc->typ))
/*JP
        what = "an altar";
*/
        what = "祭壇";
    else if (IS_DRAWBRIDGE(maploc->typ))
/*JP
        what = "a drawbridge";
*/
        what = "跳ね橋";
    else if (maploc->typ == STAIRS)
/*JP
        what = "the stairs";
*/
        what = "階段";
    else if (maploc->typ == LADDER)
/*JP
        what = "a ladder";
*/
        what = "はしご";
    else if (maploc->typ == IRONBARS)
/*JP
        what = "an iron bar";
*/
        what = "鉄棒";
    else
/*JP
        what = "something weird";
*/
        what = "何か妙なもの";
#if 0 /*JP*/
    return strcat(strcpy(buf, "kicking "), what);
#else
    Sprintf(buf, "%sを蹴って", what);
    return buf;
#endif
}

int
dokick()
{
    int x, y;
    int avrg_attrib;
    int dmg = 0, glyph, oldglyph = -1;
    register struct monst *mtmp;
    boolean no_kick = FALSE;
    char buf[BUFSZ];

    if (nolimbs(youmonst.data) || slithy(youmonst.data)) {
/*JP
        You("have no legs to kick with.");
*/
        You("何かを蹴ろうにも足がない．");
        no_kick = TRUE;
    } else if (verysmall(youmonst.data)) {
/*JP
        You("are too small to do any kicking.");
*/
        You("何かを蹴るには小さすぎる．");
        no_kick = TRUE;
    } else if (u.usteed) {
/*JP
        if (yn_function("Kick your steed?", ynchars, 'y') == 'y') {
*/
        if (yn_function("馬を蹴る？", ynchars, 'y') == 'y') {
/*JP
            You("kick %s.", mon_nam(u.usteed));
*/
            You("%sを蹴った．", mon_nam(u.usteed));
            kick_steed();
            return 1;
        } else {
            return 0;
        }
    } else if (Wounded_legs) {
        /* note: jump() has similar code */
        long wl = (EWounded_legs & BOTH_SIDES);
        const char *bp = body_part(LEG);

        if (wl == BOTH_SIDES)
            bp = makeplural(bp);
#if 0 /*JP*/
        Your("%s%s %s in no shape for kicking.",
             (wl == LEFT_SIDE) ? "left " : (wl == RIGHT_SIDE) ? "right " : "",
             bp, (wl == BOTH_SIDES) ? "are" : "is");
#else
        Your("%s%sは蹴りができる状態じゃない．",
             (wl == LEFT_SIDE) ? "左" : (wl == RIGHT_SIDE) ? "右" : "",
             bp);
#endif
        no_kick = TRUE;
    } else if (near_capacity() > SLT_ENCUMBER) {
/*JP
        Your("load is too heavy to balance yourself for a kick.");
*/
        You("たくさんものを持ちすぎて蹴りのためのバランスがとれない．");
        no_kick = TRUE;
    } else if (youmonst.data->mlet == S_LIZARD) {
/*JP
        Your("legs cannot kick effectively.");
*/
        Your("足ではうまく蹴れない．");
        no_kick = TRUE;
    } else if (u.uinwater && !rn2(2)) {
/*JP
        Your("slow motion kick doesn't hit anything.");
*/
        Your("遅い動きの蹴りでは命中しようがない．");
        no_kick = TRUE;
    } else if (u.utrap) {
        no_kick = TRUE;
        switch (u.utraptype) {
        case TT_PIT:
            if (!Passes_walls)
/*JP
                pline("There's not enough room to kick down here.");
*/
                pline("落し穴にはまっているので，蹴れない．");
            else
                no_kick = FALSE;
            break;
        case TT_WEB:
        case TT_BEARTRAP:
/*JP
            You_cant("move your %s!", body_part(LEG));
*/
            You("%sを動かすことができない！", body_part(LEG));
            break;
        default:
            break;
        }
    }

    if (no_kick) {
        /* ignore direction typed before player notices kick failed */
        display_nhwindow(WIN_MESSAGE, TRUE); /* --More-- */
        return 0;
    }

    if (!getdir((char *) 0))
        return 0;
    if (!u.dx && !u.dy)
        return 0;

    x = u.ux + u.dx;
    y = u.uy + u.dy;

    /* KMH -- Kicking boots always succeed */
    if (uarmf && uarmf->otyp == KICKING_BOOTS)
        avrg_attrib = 99;
    else
        avrg_attrib = (ACURRSTR + ACURR(A_DEX) + ACURR(A_CON)) / 3;

    if (u.uswallow) {
        switch (rn2(3)) {
        case 0:
/*JP
            You_cant("move your %s!", body_part(LEG));
*/
            You("%sを動かすことができない！", body_part(LEG));
            break;
        case 1:
            if (is_animal(u.ustuck->data)) {
/*JP
                pline("%s burps loudly.", Monnam(u.ustuck));
*/
                pline("%sは大きなゲップをした．", Monnam(u.ustuck));
                break;
            }
        default:
/*JP
            Your("feeble kick has no effect.");
*/
            Your("弱々しい蹴りは効果がない．"); break;
            break;
        }
        return 1;
    } else if (u.utrap && u.utraptype == TT_PIT) {
        /* must be Passes_walls */
/*JP
        You("kick at the side of the pit.");
*/
        You("落し穴の壁を蹴った．");
        return 1;
    }
    if (Levitation) {
        int xx, yy;

        xx = u.ux - u.dx;
        yy = u.uy - u.dy;
        /* doors can be opened while levitating, so they must be
         * reachable for bracing purposes
         * Possible extension: allow bracing against stuff on the side?
         */
        if (isok(xx, yy) && !IS_ROCK(levl[xx][yy].typ)
            && !IS_DOOR(levl[xx][yy].typ)
            && (!Is_airlevel(&u.uz) || !OBJ_AT(xx, yy))) {
/*JP
            You("have nothing to brace yourself against.");
*/
            pline("支えにできるようなものが無い．");
            return 0;
        }
    }

    mtmp = isok(x, y) ? m_at(x, y) : 0;
    /* might not kick monster if it is hidden and becomes revealed,
       if it is peaceful and player declines to attack, or if the
       hero passes out due to encumbrance with low hp; context.move
       will be 1 unless player declines to kick peaceful monster */
    if (mtmp) {
        oldglyph = glyph_at(x, y);
        if (!maybe_kick_monster(mtmp, x, y))
            return context.move;
    }

    wake_nearby();
    u_wipe_engr(2);

    if (!isok(x, y)) {
        maploc = &nowhere;
        goto ouch;
    }
    maploc = &levl[x][y];

    /*
     * The next five tests should stay in their present order:
     * monsters, pools, objects, non-doors, doors.
     *
     * [FIXME:  Monsters who are hidden underneath objects or
     * in pools should lead to hero kicking the concealment
     * rather than the monster, probably exposing the hidden
     * monster in the process.  And monsters who are hidden on
     * ceiling shouldn't be kickable (unless hero is flying?);
     * kicking toward them should just target whatever is on
     * the floor at that spot.]
     */

    if (mtmp) {
        /* save mtmp->data (for recoil) in case mtmp gets killed */
        struct permonst *mdat = mtmp->data;

        kick_monster(mtmp, x, y);
        glyph = glyph_at(x, y);
        /* see comment in attack_checks() */
        if (mtmp->mhp <= 0) { /* DEADMONSTER() */
            /* if we mapped an invisible monster and immediately
               killed it, we don't want to forget what we thought
               was there before the kick */
            if (glyph != oldglyph && glyph_is_invisible(glyph))
                show_glyph(x, y, oldglyph);
        } else if (!canspotmon(mtmp)
                   /* check <x,y>; monster that evades kick by jumping
                      to an unseen square doesn't leave an I behind */
                   && mtmp->mx == x && mtmp->my == y
                   && !glyph_is_invisible(glyph)
                   && !(u.uswallow && mtmp == u.ustuck)) {
            map_invisible(x, y);
        }
        /* recoil if floating */
        if ((Is_airlevel(&u.uz) || Levitation) && context.move) {
            int range;

            range =
                ((int) youmonst.data->cwt + (weight_cap() + inv_weight()));
            if (range < 1)
                range = 1; /* divide by zero avoidance */
            range = (3 * (int) mdat->cwt) / range;

            if (range < 1)
                range = 1;
            hurtle(-u.dx, -u.dy, range, TRUE);
        }
        return 1;
    }
    if (glyph_is_invisible(levl[x][y].glyph)) {
        unmap_object(x, y);
        newsym(x, y);
    }
    if (is_pool(x, y) ^ !!u.uinwater) {
        /* objects normally can't be removed from water by kicking */
/*JP
        You("splash some water around.");
*/
        You("水を回りにまきちらした．");
        return 1;
    }

    if (OBJ_AT(x, y) && (!Levitation || Is_airlevel(&u.uz)
                         || Is_waterlevel(&u.uz) || sobj_at(BOULDER, x, y))) {
        if (kick_object(x, y)) {
            if (Is_airlevel(&u.uz))
                hurtle(-u.dx, -u.dy, 1, TRUE); /* assume it's light */
            return 1;
        }
        goto ouch;
    }

    if (!IS_DOOR(maploc->typ)) {
        if (maploc->typ == SDOOR) {
            if (!Levitation && rn2(30) < avrg_attrib) {
                cvt_sdoor_to_door(maploc); /* ->typ = DOOR */
#if 0 /*JP*/
                pline("Crash!  %s a secret door!",
                      /* don't "kick open" when it's locked
                         unless it also happens to be trapped */
                      (maploc->doormask & (D_LOCKED | D_TRAPPED)) == D_LOCKED
                          ? "Your kick uncovers"
                          : "You kick open");
#else
                pline("ガシャン！あなたは秘密の扉を%sた！",
                      (maploc->doormask & (D_LOCKED|D_TRAPPED)) == D_LOCKED
                          ? "発見し"
                          : "蹴り開け");
#endif
                exercise(A_DEX, TRUE);
                if (maploc->doormask & D_TRAPPED) {
                    maploc->doormask = D_NODOOR;
/*JP
                    b_trapped("door", FOOT);
*/
                    b_trapped("扉", FOOT);
                } else if (maploc->doormask != D_NODOOR
                           && !(maploc->doormask & D_LOCKED))
                    maploc->doormask = D_ISOPEN;
                feel_newsym(x, y); /* we know it's gone */
                if (maploc->doormask == D_ISOPEN
                    || maploc->doormask == D_NODOOR)
                    unblock_point(x, y); /* vision */
                return 1;
            } else
                goto ouch;
        }
        if (maploc->typ == SCORR) {
            if (!Levitation && rn2(30) < avrg_attrib) {
/*JP
                pline("Crash!  You kick open a secret passage!");
*/
                pline("ガシャン！あなたは秘密の通路を蹴りやぶった！");
                exercise(A_DEX, TRUE);
                maploc->typ = CORR;
                feel_newsym(x, y); /* we know it's gone */
                unblock_point(x, y); /* vision */
                return 1;
            } else
                goto ouch;
        }
        if (IS_THRONE(maploc->typ)) {
            register int i;
            if (Levitation)
                goto dumb;
            if ((Luck < 0 || maploc->doormask) && !rn2(3)) {
                maploc->typ = ROOM;
                maploc->doormask = 0; /* don't leave loose ends.. */
                (void) mkgold((long) rnd(200), x, y);
                if (Blind)
/*JP
                    pline("CRASH!  You destroy it.");
*/
                    pline("ガシャン！あなたは何かを破壊した．");
                else {
/*JP
                    pline("CRASH!  You destroy the throne.");
*/
                    pline("ガシャン！あなたは玉座を破壊した．");
                    newsym(x, y);
                }
                exercise(A_DEX, TRUE);
                return 1;
            } else if (Luck > 0 && !rn2(3) && !maploc->looted) {
                (void) mkgold((long) rn1(201, 300), x, y);
                i = Luck + 1;
                if (i > 6)
                    i = 6;
                while (i--)
                    (void) mksobj_at(
                        rnd_class(DILITHIUM_CRYSTAL, LUCKSTONE - 1), x, y,
                        FALSE, TRUE);
                if (Blind)
/*JP
                    You("kick %s loose!", something);
*/
                    You("なにかを蹴り散らした！");
                else {
/*JP
                    You("kick loose some ornamental coins and gems!");
*/
                    You("装飾用の金貨や宝石を蹴り散らした！");
                    newsym(x, y);
                }
                /* prevent endless milking */
                maploc->looted = T_LOOTED;
                return 1;
            } else if (!rn2(4)) {
                if (dunlev(&u.uz) < dunlevs_in_dungeon(&u.uz)) {
                    fall_through(FALSE);
                    return 1;
                } else
                    goto ouch;
            }
            goto ouch;
        }
        if (IS_ALTAR(maploc->typ)) {
            if (Levitation)
                goto dumb;
/*JP
            You("kick %s.", (Blind ? something : "the altar"));
*/
            You("%sを蹴った．", (Blind ? "何か" : "祭壇"));
            if (!rn2(3))
                goto ouch;
            altar_wrath(x, y);
            exercise(A_DEX, TRUE);
            return 1;
        }
        if (IS_FOUNTAIN(maploc->typ)) {
            if (Levitation)
                goto dumb;
/*JP
            You("kick %s.", (Blind ? something : "the fountain"));
*/
            You("%sを蹴った．",(Blind ? "何か" : "泉"));
            if (!rn2(3))
                goto ouch;
            /* make metal boots rust */
            if (uarmf && rn2(3))
/*JP
                if (water_damage(uarmf, "metal boots", TRUE) == ER_NOTHING) {
*/
                if (water_damage(uarmf, "金属の靴", TRUE) == ER_NOTHING) {
/*JP
                    Your("boots get wet.");
*/
                    Your("靴は濡れた．");
                    /* could cause short-lived fumbling here */
                }
            exercise(A_DEX, TRUE);
            return 1;
        }
        if (IS_GRAVE(maploc->typ)) {
            if (Levitation)
                goto dumb;
            if (rn2(4))
                goto ouch;
            exercise(A_WIS, FALSE);
            if (Role_if(PM_ARCHEOLOGIST) || Role_if(PM_SAMURAI)
                || ((u.ualign.type == A_LAWFUL) && (u.ualign.record > -10))) {
                adjalign(-sgn(u.ualign.type));
            }
            maploc->typ = ROOM;
            maploc->doormask = 0;
            (void) mksobj_at(ROCK, x, y, TRUE, FALSE);
            del_engr_at(x, y);
            if (Blind)
#if 0 /*JP*/
                pline("Crack!  %s broke!", Something);
#else
                pline("ゴツン！何かが壊れた！");
#endif
            else {
/*JP
                pline_The("headstone topples over and breaks!");
*/
                pline("墓石は倒れて壊れた！");
                newsym(x, y);
            }
            return 1;
        }
        if (maploc->typ == IRONBARS)
            goto ouch;
        if (IS_TREE(maploc->typ)) {
            struct obj *treefruit;
            /* nothing, fruit or trouble? 75:23.5:1.5% */
            if (rn2(3)) {
                if (!rn2(6) && !(mvitals[PM_KILLER_BEE].mvflags & G_GONE))
#if 0 /*JP*/
                    You_hear("a low buzzing."); /* a warning */
#else
                    You_hear("ぶーんという音を聞いた．"); /* a warning */
#endif
                goto ouch;
            }
            if (rn2(15) && !(maploc->looted & TREE_LOOTED)
                && (treefruit = rnd_treefruit_at(x, y))) {
                long nfruit = 8L - rnl(7), nfall;
                short frtype = treefruit->otyp;
                treefruit->quan = nfruit;
                if (is_plural(treefruit))
/*JP
                    pline("Some %s fall from the tree!", xname(treefruit));
*/
                    pline("%sが何個か木から落ちてきた！", xname(treefruit));
                else
/*JP
                    pline("%s falls from the tree!", An(xname(treefruit)));
*/
                    pline("%sが木から落ちてきた！", xname(treefruit));
                nfall = scatter(x, y, 2, MAY_HIT, treefruit);
                if (nfall != nfruit) {
                    /* scatter left some in the tree, but treefruit
                     * may not refer to the correct object */
                    treefruit = mksobj(frtype, TRUE, FALSE);
                    treefruit->quan = nfruit - nfall;
#if 0 /*JP*/
                    pline("%ld %s got caught in the branches.",
                          nfruit - nfall, xname(treefruit));
#else
                    pline("%ld個の%sが枝にひっかかった．",
                          nfruit - nfall, xname(treefruit));
#endif
                    dealloc_obj(treefruit);
                }
                exercise(A_DEX, TRUE);
                exercise(A_WIS, TRUE); /* discovered a new food source! */
                newsym(x, y);
                maploc->looted |= TREE_LOOTED;
                return 1;
            } else if (!(maploc->looted & TREE_SWARM)) {
                int cnt = rnl(4) + 2;
                int made = 0;
                coord mm;
                mm.x = x;
                mm.y = y;
                while (cnt--) {
                    if (enexto(&mm, mm.x, mm.y, &mons[PM_KILLER_BEE])
                        && makemon(&mons[PM_KILLER_BEE], mm.x, mm.y,
                                   MM_ANGRY))
                        made++;
                }
                if (made)
/*JP
                    pline("You've attracted the tree's former occupants!");
*/
                    pline("木の先住民を起こしてしまった！");
                else
/*JP
                    You("smell stale honey.");
*/
                    pline("古いはちみつのにおいがした．");
                maploc->looted |= TREE_SWARM;
                return 1;
            }
            goto ouch;
        }
        if (IS_SINK(maploc->typ)) {
            int gend = poly_gender();
            short washerndx = (gend == 1 || (gend == 2 && rn2(2)))
                                  ? PM_INCUBUS
                                  : PM_SUCCUBUS;

            if (Levitation)
                goto dumb;
            if (rn2(5)) {
                if (!Deaf)
/*JP
                    pline("Klunk!  The pipes vibrate noisily.");
*/
                    pline("ガラン！パイプはうるさく振動した．");
                else
/*JP
                    pline("Klunk!");
*/
                    pline("ガラン！");
                exercise(A_DEX, TRUE);
                return 1;
            } else if (!(maploc->looted & S_LPUDDING) && !rn2(3)
                       && !(mvitals[PM_BLACK_PUDDING].mvflags & G_GONE)) {
                if (Blind)
/*JP
                    You_hear("a gushing sound.");
*/
                    You_hear("なにかが噴出する音を聞いた．");
                else
/*JP
                    pline("A %s ooze gushes up from the drain!",
*/
                    pline("%s液体が排水口からにじみ出た！",
                          hcolor(NH_BLACK));
                (void) makemon(&mons[PM_BLACK_PUDDING], x, y, NO_MM_FLAGS);
                exercise(A_DEX, TRUE);
                newsym(x, y);
                maploc->looted |= S_LPUDDING;
                return 1;
            } else if (!(maploc->looted & S_LDWASHER) && !rn2(3)
                       && !(mvitals[washerndx].mvflags & G_GONE)) {
                /* can't resist... */
/*JP
                pline("%s returns!", (Blind ? Something : "The dish washer"));
*/
                pline("%sが戻ってきた！", (Blind ? "何か" : "皿洗い"));
                if (makemon(&mons[washerndx], x, y, NO_MM_FLAGS))
                    newsym(x, y);
                maploc->looted |= S_LDWASHER;
                exercise(A_DEX, TRUE);
                return 1;
            } else if (!rn2(3)) {
#if 0 /*JP*/
                pline("Flupp!  %s.",
                      (Blind ? "You hear a sloshing sound"
                             : "Muddy waste pops up from the drain"));
#else
                pline("うわ！%s．",
                      (Blind ? "あなたは，バチャバチャする音を聞いた"
                             : "排水口からどろどろの廃棄物が出てくる"));
#endif
                if (!(maploc->looted & S_LRING)) { /* once per sink */
                    if (!Blind)
/*JP
                        You_see("a ring shining in its midst.");
*/
                        You("その中に光る指輪を見つけた．");
                    (void) mkobj_at(RING_CLASS, x, y, TRUE);
                    newsym(x, y);
                    exercise(A_DEX, TRUE);
                    exercise(A_WIS, TRUE); /* a discovery! */
                    maploc->looted |= S_LRING;
                }
                return 1;
            }
            goto ouch;
        }
        if (maploc->typ == STAIRS || maploc->typ == LADDER
            || IS_STWALL(maploc->typ)) {
            if (!IS_STWALL(maploc->typ) && maploc->ladder == LA_DOWN)
                goto dumb;
        ouch:
/*JP
            pline("Ouch!  That hurts!");
*/
            pline("いてっ！怪我した！");
            exercise(A_DEX, FALSE);
            exercise(A_STR, FALSE);
            if (isok(x, y)) {
                if (Blind)
                    feel_location(x, y); /* we know we hit it */
                if (is_drawbridge_wall(x, y) >= 0) {
/*JP
                    pline_The("drawbridge is unaffected.");
*/
                    pline("跳ね橋はびくともしない．");
                    /* update maploc to refer to the drawbridge */
                    (void) find_drawbridge(&x, &y);
                    maploc = &levl[x][y];
                }
            }
            if (!rn2(3))
                set_wounded_legs(RIGHT_SIDE, 5 + rnd(5));
            dmg = rnd(ACURR(A_CON) > 15 ? 3 : 5);
            losehp(Maybe_Half_Phys(dmg), kickstr(buf), KILLED_BY);
            if (Is_airlevel(&u.uz) || Levitation)
                hurtle(-u.dx, -u.dy, rn1(2, 4), TRUE); /* assume it's heavy */
            return 1;
        }
        goto dumb;
    }

    if (maploc->doormask == D_ISOPEN || maploc->doormask == D_BROKEN
        || maploc->doormask == D_NODOOR) {
    dumb:
        exercise(A_DEX, FALSE);
        if (martial() || ACURR(A_DEX) >= 16 || rn2(3)) {
/*JP
            You("kick at empty space.");
*/
            You("何もない空間を蹴った．");
            if (Blind)
                feel_location(x, y);
        } else {
/*JP
            pline("Dumb move!  You strain a muscle.");
*/
            pline("ばかげた動きだ！筋肉を痛めた．");
            exercise(A_STR, FALSE);
            set_wounded_legs(RIGHT_SIDE, 5 + rnd(5));
        }
        if ((Is_airlevel(&u.uz) || Levitation) && rn2(2))
            hurtle(-u.dx, -u.dy, 1, TRUE);
        return 1; /* uses a turn */
    }

    /* not enough leverage to kick open doors while levitating */
    if (Levitation)
        goto ouch;

    exercise(A_DEX, TRUE);
    /* door is known to be CLOSED or LOCKED */
    if (rnl(35) < avrg_attrib + (!martial() ? 0 : ACURR(A_DEX))) {
        boolean shopdoor = *in_rooms(x, y, SHOPBASE) ? TRUE : FALSE;
        /* break the door */
        if (maploc->doormask & D_TRAPPED) {
            if (flags.verbose)
/*JP
                You("kick the door.");
*/
                You("扉を蹴った．");
            exercise(A_STR, FALSE);
            maploc->doormask = D_NODOOR;
/*JP
            b_trapped("door", FOOT);
*/
            b_trapped("扉", FOOT);
        } else if (ACURR(A_STR) > 18 && !rn2(5) && !shopdoor) {
/*JP
            pline("As you kick the door, it shatters to pieces!");
*/
            pline("扉を蹴ると，こなごなにくだけた！");
            exercise(A_STR, TRUE);
            maploc->doormask = D_NODOOR;
        } else {
/*JP
            pline("As you kick the door, it crashes open!");
*/
            pline("扉を蹴ると，壊れて開いた！");
            exercise(A_STR, TRUE);
            maploc->doormask = D_BROKEN;
        }
        feel_newsym(x, y); /* we know we broke it */
        unblock_point(x, y); /* vision */
        if (shopdoor) {
            add_damage(x, y, 400L);
/*JP
            pay_for_damage("break", FALSE);
*/
            pay_for_damage("破壊する", FALSE);
        }
        if (in_town(x, y))
            for (mtmp = fmon; mtmp; mtmp = mtmp->nmon) {
                if (DEADMONSTER(mtmp))
                    continue;
                if (is_watch(mtmp->data) && couldsee(mtmp->mx, mtmp->my)
                    && mtmp->mpeaceful) {
/*JP
                    mon_yells(mtmp, "Halt, thief!  You're under arrest!");
*/
                    mon_yells(mtmp, "止まれ泥棒！おまえを逮捕する！");
                    (void) angry_guards(FALSE);
                    break;
                }
            }
    } else {
        if (Blind)
            feel_location(x, y); /* we know we hit it */
        exercise(A_STR, TRUE);
/*JP
        pline("WHAMMM!!!");
*/
        pline("ぐぁぁぁん！");
        if (in_town(x, y))
            for (mtmp = fmon; mtmp; mtmp = mtmp->nmon) {
                if (DEADMONSTER(mtmp))
                    continue;
                if (is_watch(mtmp->data) && mtmp->mpeaceful
                    && couldsee(mtmp->mx, mtmp->my)) {
                    if (levl[x][y].looted & D_WARNED) {
                        mon_yells(mtmp,
/*JP
                                  "Halt, vandal!  You're under arrest!");
*/
                                  "止まれ野蛮人！おまえを逮捕する！");
                        (void) angry_guards(FALSE);
                    } else {
/*JP
                        mon_yells(mtmp, "Hey, stop damaging that door!");
*/
                        mon_yells(mtmp, "おい，扉を破壊するのをやめろ！");
                        levl[x][y].looted |= D_WARNED;
                    }
                    break;
                }
            }
    }
    return 1;
}

STATIC_OVL void
drop_to(cc, loc)
coord *cc;
schar loc;
{
    /* cover all the MIGR_xxx choices generated by down_gate() */
    switch (loc) {
    case MIGR_RANDOM: /* trap door or hole */
        if (Is_stronghold(&u.uz)) {
            cc->x = valley_level.dnum;
            cc->y = valley_level.dlevel;
            break;
        } else if (In_endgame(&u.uz) || Is_botlevel(&u.uz)) {
            cc->y = cc->x = 0;
            break;
        } /* else fall to the next cases */
    case MIGR_STAIRS_UP:
    case MIGR_LADDER_UP:
        cc->x = u.uz.dnum;
        cc->y = u.uz.dlevel + 1;
        break;
    case MIGR_SSTAIRS:
        cc->x = sstairs.tolev.dnum;
        cc->y = sstairs.tolev.dlevel;
        break;
    default:
    case MIGR_NOWHERE:
        /* y==0 means "nowhere", in which case x doesn't matter */
        cc->y = cc->x = 0;
        break;
    }
}

/* player or missile impacts location, causing objects to fall down */
void
impact_drop(missile, x, y, dlev)
struct obj *missile; /* caused impact, won't drop itself */
xchar x, y;          /* location affected */
xchar dlev;          /* if !0 send to dlev near player */
{
    schar toloc;
    register struct obj *obj, *obj2;
    register struct monst *shkp;
    long oct, dct, price, debit, robbed;
    boolean angry, costly, isrock;
    coord cc;

    if (!OBJ_AT(x, y))
        return;

    toloc = down_gate(x, y);
    drop_to(&cc, toloc);
    if (!cc.y)
        return;

    if (dlev) {
        /* send objects next to player falling through trap door.
         * checked in obj_delivery().
         */
        toloc = MIGR_WITH_HERO;
        cc.y = dlev;
    }

    costly = costly_spot(x, y);
    price = debit = robbed = 0L;
    angry = FALSE;
    shkp = (struct monst *) 0;
    /* if 'costly', we must keep a record of ESHK(shkp) before
     * it undergoes changes through the calls to stolen_value.
     * the angry bit must be reset, if needed, in this fn, since
     * stolen_value is called under the 'silent' flag to avoid
     * unsavory pline repetitions.
     */
    if (costly) {
        if ((shkp = shop_keeper(*in_rooms(x, y, SHOPBASE))) != 0) {
            debit = ESHK(shkp)->debit;
            robbed = ESHK(shkp)->robbed;
            angry = !shkp->mpeaceful;
        }
    }

    isrock = (missile && missile->otyp == ROCK);
    oct = dct = 0L;
    for (obj = level.objects[x][y]; obj; obj = obj2) {
        obj2 = obj->nexthere;
        if (obj == missile)
            continue;
        /* number of objects in the pile */
        oct += obj->quan;
        if (obj == uball || obj == uchain)
            continue;
        /* boulders can fall too, but rarely & never due to rocks */
        if ((isrock && obj->otyp == BOULDER)
            || rn2(obj->otyp == BOULDER ? 30 : 3))
            continue;
        obj_extract_self(obj);

        if (costly) {
            price += stolen_value(
                obj, x, y, (costly_spot(u.ux, u.uy)
                            && index(u.urooms, *in_rooms(x, y, SHOPBASE))),
                TRUE);
            /* set obj->no_charge to 0 */
            if (Has_contents(obj))
                picked_container(obj); /* does the right thing */
            if (obj->oclass != COIN_CLASS)
                obj->no_charge = 0;
        }

        add_to_migration(obj);
        obj->ox = cc.x;
        obj->oy = cc.y;
        obj->owornmask = (long) toloc;

        /* number of fallen objects */
        dct += obj->quan;
    }

    if (dct && cansee(x, y)) { /* at least one object fell */
/*JP
        const char *what = (dct == 1L ? "object falls" : "objects fall");
*/
        const char *what = "物";

        if (missile)
#if 0 /*JP:T*/
            pline("From the impact, %sother %s.",
                  dct == oct ? "the " : dct == 1L ? "an" : "", what);
#else
            pline("衝撃で，他の%sが落ちた．",what);
#endif
        else if (oct == dct)
#if 0 /*JP:T*/
            pline("%s adjacent %s %s.", dct == 1L ? "The" : "All the", what,
                  gate_str);
#else
            pline("近くにあった%sが%s落ちた．", what, gate_str);
#endif
        else
#if 0 /*JP*/
            pline("%s adjacent %s %s.",
                  dct == 1L ? "One of the" : "Some of the",
                  dct == 1L ? "objects falls" : what, gate_str);
#else
            pline("近くにあった%s%s%s落ちた．",
                  what,
                  dct == 1L ? "が" : "のいくつかが",
                  gate_str);
#endif
    }

    if (costly && shkp && price) {
        if (ESHK(shkp)->robbed > robbed) {
/*JP
            You("removed %ld %s worth of goods!", price, currency(price));
*/
            You("%ld%s分の品物を取りさった！", price, currency(price));
            if (cansee(shkp->mx, shkp->my)) {
                if (ESHK(shkp)->customer[0] == 0)
                    (void) strncpy(ESHK(shkp)->customer, plname, PL_NSIZ);
                if (angry)
/*JP
                    pline("%s is infuriated!", Monnam(shkp));
*/
                    pline("%sは激怒した！", Monnam(shkp));
                else
/*JP
                    pline("\"%s, you are a thief!\"", plname);
*/
                    pline("「%sめ，おまえは泥棒だな！」", plname);
            } else
/*JP
                You_hear("a scream, \"Thief!\"");
*/
                You_hear("金切り声を聞いた「泥棒！」");
            hot_pursuit(shkp);
            (void) angry_guards(FALSE);
            return;
        }
        if (ESHK(shkp)->debit > debit) {
            long amt = (ESHK(shkp)->debit - debit);
#if 0 /*JP*/
            You("owe %s %ld %s for goods lost.", Monnam(shkp), amt,
                currency(amt));
#else
            You("品物消失のため%sに%ld%sの借りをつくった．", Monnam(shkp), amt,
                currency(amt));
#endif
        }
    }
}

/* NOTE: ship_object assumes otmp was FREED from fobj or invent.
 * <x,y> is the point of drop.  otmp is _not_ an <x,y> resident:
 * otmp is either a kicked, dropped, or thrown object.
 */
boolean
ship_object(otmp, x, y, shop_floor_obj)
xchar x, y;
struct obj *otmp;
boolean shop_floor_obj;
{
    schar toloc;
    xchar ox, oy;
    coord cc;
    struct obj *obj;
    struct trap *t;
    boolean nodrop, unpaid, container, impact = FALSE;
    long n = 0L;

    if (!otmp)
        return FALSE;
    if ((toloc = down_gate(x, y)) == MIGR_NOWHERE)
        return FALSE;
    drop_to(&cc, toloc);
    if (!cc.y)
        return FALSE;

    /* objects other than attached iron ball always fall down ladder,
       but have a chance of staying otherwise */
    nodrop = (otmp == uball) || (otmp == uchain)
             || (toloc != MIGR_LADDER_UP && rn2(3));

    container = Has_contents(otmp);
    unpaid = is_unpaid(otmp);

    if (OBJ_AT(x, y)) {
        for (obj = level.objects[x][y]; obj; obj = obj->nexthere)
            if (obj != otmp)
                n += obj->quan;
        if (n)
            impact = TRUE;
    }
    /* boulders never fall through trap doors, but they might knock
       other things down before plugging the hole */
    if (otmp->otyp == BOULDER && ((t = t_at(x, y)) != 0)
        && (t->ttyp == TRAPDOOR || t->ttyp == HOLE)) {
        if (impact)
            impact_drop(otmp, x, y, 0);
        return FALSE; /* let caller finish the drop */
    }

    if (cansee(x, y))
        otransit_msg(otmp, nodrop, n);

    if (nodrop) {
        if (impact)
            impact_drop(otmp, x, y, 0);
        return FALSE;
    }

    if (unpaid || shop_floor_obj) {
        if (unpaid) {
            (void) stolen_value(otmp, u.ux, u.uy, TRUE, FALSE);
        } else {
            ox = otmp->ox;
            oy = otmp->oy;
            (void) stolen_value(
                otmp, ox, oy,
                (costly_spot(u.ux, u.uy)
                 && index(u.urooms, *in_rooms(ox, oy, SHOPBASE))),
                FALSE);
        }
        /* set otmp->no_charge to 0 */
        if (container)
            picked_container(otmp); /* happens to do the right thing */
        if (otmp->oclass != COIN_CLASS)
            otmp->no_charge = 0;
    }

    if (otmp->owornmask)
        remove_worn_item(otmp, TRUE);

    /* some things break rather than ship */
    if (breaktest(otmp)) {
        const char *result;

        if (objects[otmp->otyp].oc_material == GLASS
            || otmp->otyp == EXPENSIVE_CAMERA) {
            if (otmp->otyp == MIRROR)
                change_luck(-2);
/*JP
            result = "crash";
*/
            result = "グシャッ";
        } else {
            /* penalty for breaking eggs laid by you */
            if (otmp->otyp == EGG && otmp->spe && otmp->corpsenm >= LOW_PM)
                change_luck((schar) -min(otmp->quan, 5L));
/*JP
            result = "splat";
*/
            result = "ベチャッ";
        }
/*JP
        You_hear("a muffled %s.", result);
*/
        You_hear("こもった%sという音を聞いた．", result);
        obj_extract_self(otmp);
        obfree(otmp, (struct obj *) 0);
        return TRUE;
    }

    add_to_migration(otmp);
    otmp->ox = cc.x;
    otmp->oy = cc.y;
    otmp->owornmask = (long) toloc;
    /* boulder from rolling boulder trap, no longer part of the trap */
    if (otmp->otyp == BOULDER)
        otmp->otrapped = 0;

    if (impact) {
        /* the objs impacted may be in a shop other than
         * the one in which the hero is located.  another
         * check for a shk is made in impact_drop.  it is, e.g.,
         * possible to kick/throw an object belonging to one
         * shop into another shop through a gap in the wall,
         * and cause objects belonging to the other shop to
         * fall down a trap door--thereby getting two shopkeepers
         * angry at the hero in one shot.
         */
        impact_drop(otmp, x, y, 0);
        newsym(x, y);
    }
    return TRUE;
}

void
obj_delivery(near_hero)
boolean near_hero;
{
    register struct obj *otmp, *otmp2;
    register int nx, ny;
    int where;
    boolean nobreak, noscatter;

    for (otmp = migrating_objs; otmp; otmp = otmp2) {
        otmp2 = otmp->nobj;
        if (otmp->ox != u.uz.dnum || otmp->oy != u.uz.dlevel)
            continue;

        where = (int) (otmp->owornmask & 0x7fffL); /* destination code */
        nobreak = (where & MIGR_NOBREAK) != 0;
        noscatter = (where & MIGR_WITH_HERO) != 0;
        where &= ~(MIGR_NOBREAK | MIGR_NOSCATTER);

        if (!near_hero ^ (where == MIGR_WITH_HERO))
            continue;

        obj_extract_self(otmp);
        otmp->owornmask = 0L;

        switch (where) {
        case MIGR_STAIRS_UP:
            nx = xupstair, ny = yupstair;
            break;
        case MIGR_LADDER_UP:
            nx = xupladder, ny = yupladder;
            break;
        case MIGR_SSTAIRS:
            nx = sstairs.sx, ny = sstairs.sy;
            break;
        case MIGR_WITH_HERO:
            nx = u.ux, ny = u.uy;
            break;
        default:
        case MIGR_RANDOM:
            nx = ny = 0;
            break;
        }
        if (nx > 0) {
            place_object(otmp, nx, ny);
            if (!nobreak && !IS_SOFT(levl[nx][ny].typ)) {
                if (where == MIGR_WITH_HERO) {
                    if (breaks(otmp, nx, ny))
                        continue;
                } else if (breaktest(otmp)) {
                    /* assume it broke before player arrived, no messages */
                    delobj(otmp);
                    continue;
                }
            }
            stackobj(otmp);
            if (!noscatter)
                (void) scatter(nx, ny, rnd(2), 0, otmp);
        } else { /* random location */
            /* set dummy coordinates because there's no
               current position for rloco() to update */
            otmp->ox = otmp->oy = 0;
            if (rloco(otmp) && !nobreak && breaktest(otmp)) {
                /* assume it broke before player arrived, no messages */
                delobj(otmp);
            }
        }
    }
}

STATIC_OVL void
otransit_msg(otmp, nodrop, num)
register struct obj *otmp;
register boolean nodrop;
long num;
{
    char obuf[BUFSZ];

#if 0 /*JP*/
    Sprintf(obuf, "%s%s",
            (otmp->otyp == CORPSE && type_is_pname(&mons[otmp->corpsenm]))
                ? ""
                : "The ",
            cxname(otmp));
#else
    Sprintf(obuf, "%sは", xname(otmp));
#endif

    if (num) { /* means: other objects are impacted */
#if 0 /*JP*/
        Sprintf(eos(obuf), " %s %s object%s", otense(otmp, "hit"),
                num == 1L ? "another" : "other", num > 1L ? "s" : "");
        if (nodrop)
            Sprintf(eos(obuf), ".");
        else
            Sprintf(eos(obuf), " and %s %s.", otense(otmp, "fall"), gate_str);
#else
        Sprintf(eos(obuf), "他の物体に命中して");
        if(nodrop)
            Sprintf(eos(obuf), "止まった．");
        else
            Sprintf(eos(obuf), "%s落ちた．", gate_str);
#endif
        pline1(obuf);
    } else if (!nodrop)
/*JP
        pline("%s %s %s.", obuf, otense(otmp, "fall"), gate_str);
*/
        pline("%s%s落ちた．", obuf, gate_str);
}

/* migration destination for objects which fall down to next level */
schar
down_gate(x, y)
xchar x, y;
{
    struct trap *ttmp;

    gate_str = 0;
    /* this matches the player restriction in goto_level() */
    if (on_level(&u.uz, &qstart_level) && !ok_to_quest())
        return MIGR_NOWHERE;

    if ((xdnstair == x && ydnstair == y)
        || (sstairs.sx == x && sstairs.sy == y && !sstairs.up)) {
/*JP
        gate_str = "down the stairs";
*/
        gate_str = "階段から";
        return (xdnstair == x && ydnstair == y) ? MIGR_STAIRS_UP
                                                : MIGR_SSTAIRS;
    }
    if (xdnladder == x && ydnladder == y) {
/*JP
        gate_str = "down the ladder";
*/
        gate_str = "はしごから";
        return MIGR_LADDER_UP;
    }

    if (((ttmp = t_at(x, y)) != 0 && ttmp->tseen)
        && (ttmp->ttyp == TRAPDOOR || ttmp->ttyp == HOLE)) {
#if 0 /*JP*/
        gate_str = (ttmp->ttyp == TRAPDOOR) ? "through the trap door"
                                            : "through the hole";
#else
        gate_str = (ttmp->ttyp == TRAPDOOR) ? "落し扉に"
                                            : "穴に";
#endif
        return MIGR_RANDOM;
    }
    return MIGR_NOWHERE;
}

/*dokick.c*/

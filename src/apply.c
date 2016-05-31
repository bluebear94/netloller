/* NetHack 3.6	apply.c	$NHDT-Date: 1446808436 2015/11/06 11:13:56 $  $NHDT-Branch: master $:$NHDT-Revision: 1.210 $ */
/* Copyright (c) Stichting Mathematisch Centrum, Amsterdam, 1985. */
/* NetHack may be freely redistributed.  See license for details. */

/* JNetHack Copyright */
/* (c) Issei Numata, Naoki Hamada, Shigehiro Miyashita, 1994-2000  */
/* For 3.4-, Copyright (c) SHIRAKATA Kentaro, 2002-2016            */
/* JNetHack may be freely redistributed.  See license for details. */

#include "hack.h"

extern boolean notonhead; /* for long worms */

STATIC_DCL int FDECL(use_camera, (struct obj *));
STATIC_DCL int FDECL(use_towel, (struct obj *));
STATIC_DCL boolean FDECL(its_dead, (int, int, int *));
STATIC_DCL int FDECL(use_stethoscope, (struct obj *));
STATIC_DCL void FDECL(use_whistle, (struct obj *));
STATIC_DCL void FDECL(use_magic_whistle, (struct obj *));
STATIC_DCL void FDECL(use_leash, (struct obj *));
STATIC_DCL int FDECL(use_mirror, (struct obj *));
STATIC_DCL void FDECL(use_bell, (struct obj **));
STATIC_DCL void FDECL(use_candelabrum, (struct obj *));
STATIC_DCL void FDECL(use_candle, (struct obj **));
STATIC_DCL void FDECL(use_lamp, (struct obj *));
STATIC_DCL void FDECL(light_cocktail, (struct obj *));
STATIC_PTR void FDECL(display_jump_positions, (int));
STATIC_DCL void FDECL(use_tinning_kit, (struct obj *));
STATIC_DCL void FDECL(use_figurine, (struct obj **));
STATIC_DCL void FDECL(use_grease, (struct obj *));
STATIC_DCL void FDECL(use_trap, (struct obj *));
STATIC_DCL void FDECL(use_stone, (struct obj *));
STATIC_PTR int NDECL(set_trap); /* occupation callback */
STATIC_DCL int FDECL(use_whip, (struct obj *));
STATIC_PTR void FDECL(display_polearm_positions, (int));
STATIC_DCL int FDECL(use_pole, (struct obj *));
STATIC_DCL int FDECL(use_cream_pie, (struct obj *));
STATIC_DCL int FDECL(use_grapple, (struct obj *));
STATIC_DCL int FDECL(do_break_wand, (struct obj *));
STATIC_DCL boolean FDECL(figurine_location_checks, (struct obj *,
                                                    coord *, BOOLEAN_P));
STATIC_DCL void FDECL(add_class, (char *, CHAR_P));
STATIC_DCL void FDECL(setapplyclasses, (char *));
STATIC_DCL boolean FDECL(is_valid_jump_pos, (int, int, int, BOOLEAN_P));
STATIC_DCL boolean FDECL(find_poleable_mon, (coord *, int, int));

#ifdef AMIGA
void FDECL(amii_speaker, (struct obj *, char *, int));
#endif

static const char no_elbow_room[] =
/*JP
    "don't have enough elbow-room to maneuver.";
*/
    "それをするだけのゆとりがない．";

STATIC_OVL int
use_camera(obj)
struct obj *obj;
{
    struct monst *mtmp;

    if (Underwater) {
/*JP
        pline("Using your camera underwater would void the warranty.");
*/
        pline("水面下でのカメラの使用は保証の対象外です．");
        return 0;
    }
    if (!getdir((char *) 0))
        return 0;

    if (obj->spe <= 0) {
        pline1(nothing_happens);
        return 1;
    }
    consume_obj_charge(obj, TRUE);

    if (obj->cursed && !rn2(2)) {
        (void) zapyourself(obj, TRUE);
    } else if (u.uswallow) {
#if 0 /*JP*/
        You("take a picture of %s %s.", s_suffix(mon_nam(u.ustuck)),
            mbodypart(u.ustuck, STOMACH));
#else
        You("%sの%sの写真を撮った．", mon_nam(u.ustuck),
            mbodypart(u.ustuck, STOMACH));
#endif
    } else if (u.dz) {
/*JP
        You("take a picture of the %s.",
*/
        You("%sの写真を撮った．",
            (u.dz > 0) ? surface(u.ux, u.uy) : ceiling(u.ux, u.uy));
    } else if (!u.dx && !u.dy) {
        (void) zapyourself(obj, TRUE);
    } else if ((mtmp = bhit(u.dx, u.dy, COLNO, FLASHED_LIGHT,
                            (int FDECL((*), (MONST_P, OBJ_P))) 0,
                            (int FDECL((*), (OBJ_P, OBJ_P))) 0, &obj)) != 0) {
        obj->ox = u.ux, obj->oy = u.uy;
        (void) flash_hits_mon(mtmp, obj);
    }
    return 1;
}

STATIC_OVL int
use_towel(obj)
struct obj *obj;
{
    boolean drying_feedback = (obj == uwep);

    if (!freehand()) {
/*JP
        You("have no free %s!", body_part(HAND));
*/
        You("%sの自由が効かない！", body_part(HAND));
        return 0;
    } else if (obj == ublindf) {
/*JP
        You("cannot use it while you're wearing it!");
*/
        You("それを身につけているので使用できない！");
        return 0;
    } else if (obj->cursed) {
        long old;

        switch (rn2(3)) {
        case 2:
            old = Glib;
            incr_itimeout(&Glib, rn1(10, 3));
#if 0 /*JP*/
            Your("%s %s!", makeplural(body_part(HAND)),
                 (old ? "are filthier than ever" : "get slimy"));
#else
            Your("%sは%s！", makeplural(body_part(HAND)),
                 (old ? "ますます汚なくなった" : "ぬるぬるになった"));
#endif
            if (is_wet_towel(obj))
                dry_a_towel(obj, -1, drying_feedback);
            return 1;
        case 1:
            if (!ublindf) {
                old = u.ucreamed;
                u.ucreamed += rn1(10, 3);
#if 0 /*JP*/
                pline("Yecch! Your %s %s gunk on it!", body_part(FACE),
                      (old ? "has more" : "now has"));
#else
                pline("ゲェー！あなたの%sは%sべとべとになった！", body_part(FACE),
                      (old ? "もっと" : ""));
#endif
                make_blinded(Blinded + (long) u.ucreamed - old, TRUE);
            } else {
                const char *what;

#if 0 /*JP*/
                what = (ublindf->otyp == LENSES)
                           ? "lenses"
                           : (obj->otyp == ublindf->otyp) ? "other towel"
                                                          : "blindfold";
#else
                what = (ublindf->otyp == LENSES)
                           ? "レンズ"
                           : (obj->otyp == ublindf->otyp) ? "タオル"
                                                          : "目隠し";
#endif
                if (ublindf->cursed) {
#if 0 /*JP*/
                    You("push your %s %s.", what,
                        rn2(2) ? "cock-eyed" : "crooked");
#else
                    pline("%sが%s．", what,
                        rn2(2) ? "ずれた" : "ゆがんだ");
#endif
                } else {
                    struct obj *saved_ublindf = ublindf;
/*JP
                    You("push your %s off.", what);
*/
                    pline("%sがずり落ちた．", what);
                    Blindf_off(ublindf);
                    dropx(saved_ublindf);
                }
            }
            if (is_wet_towel(obj))
                dry_a_towel(obj, -1, drying_feedback);
            return 1;
        case 0:
            break;
        }
    }

    if (Glib) {
        Glib = 0;
/*JP
        You("wipe off your %s.", makeplural(body_part(HAND)));
*/
        You("%sを拭いた．", makeplural(body_part(HAND)));
        if (is_wet_towel(obj))
            dry_a_towel(obj, -1, drying_feedback);
        return 1;
    } else if (u.ucreamed) {
        Blinded -= u.ucreamed;
        u.ucreamed = 0;
        if (!Blinded) {
/*JP
            pline("You've got the glop off.");
*/
            You("さっぱりした．");
            if (!gulp_blnd_check()) {
                Blinded = 1;
                make_blinded(0L, TRUE);
            }
        } else {
/*JP
            Your("%s feels clean now.", body_part(FACE));
*/
            pline("%sの汚れを拭きとった．", body_part(FACE));
        }
        if (is_wet_towel(obj))
            dry_a_towel(obj, -1, drying_feedback);
        return 1;
    }

#if 0 /*JP*/
    Your("%s and %s are already clean.", body_part(FACE),
         makeplural(body_part(HAND)));
#else
    Your("%sや%sは汚れていない．", body_part(FACE),
         makeplural(body_part(HAND)));
#endif

    return 0;
}

/* maybe give a stethoscope message based on floor objects */
STATIC_OVL boolean
its_dead(rx, ry, resp)
int rx, ry, *resp;
{
    char buf[BUFSZ];
    boolean more_corpses;
    struct permonst *mptr;
    struct obj *corpse = sobj_at(CORPSE, rx, ry),
               *statue = sobj_at(STATUE, rx, ry);

    if (!can_reach_floor(TRUE)) { /* levitation or unskilled riding */
        corpse = 0;               /* can't reach corpse on floor */
        /* you can't reach tiny statues (even though you can fight
           tiny monsters while levitating--consistency, what's that?) */
        while (statue && mons[statue->corpsenm].msize == MZ_TINY)
            statue = nxtobj(statue, STATUE, TRUE);
    }
    /* when both corpse and statue are present, pick the uppermost one */
    if (corpse && statue) {
        if (nxtobj(statue, CORPSE, TRUE) == corpse)
            corpse = 0; /* corpse follows statue; ignore it */
        else
            statue = 0; /* corpse precedes statue; ignore statue */
    }
    more_corpses = (corpse && nxtobj(corpse, CORPSE, TRUE));

    /* additional stethoscope messages from jyoung@apanix.apana.org.au */
    if (!corpse && !statue) {
        ; /* nothing to do */

    } else if (Hallucination) {
        if (!corpse) {
            /* it's a statue */
/*JP
            Strcpy(buf, "You're both stoned");
*/
            Strcpy(buf, "石だ");
#if 0 /*JP*//*代名詞処理は不要*/
        } else if (corpse->quan == 1L && !more_corpses) {
            int gndr = 2; /* neuter: "it" */
            struct monst *mtmp = get_mtraits(corpse, FALSE);

            /* (most corpses don't retain the monster's sex, so
               we're usually forced to use generic pronoun here) */
            if (mtmp) {
                mptr = &mons[mtmp->mnum];
                /* can't use mhe() here; it calls pronoun_gender() which
                   expects monster to be on the map (visibility check) */
                if ((humanoid(mptr) || (mptr->geno & G_UNIQ)
                     || type_is_pname(mptr)) && !is_neuter(mptr))
                    gndr = (int) mtmp->female;
            } else {
                mptr = &mons[corpse->corpsenm];
                if (is_female(mptr))
                    gndr = 1;
                else if (is_male(mptr))
                    gndr = 0;
            }
            Sprintf(buf, "%s's dead", genders[gndr].he); /* "he"/"she"/"it" */
            buf[0] = highc(buf[0]);
#endif
        } else { /* plural */
/*JP
            Strcpy(buf, "They're dead");
*/
            Strcpy(buf, "死んでるぜ");
        }
        /* variations on "He's dead, Jim." (Star Trek's Dr McCoy) */
/*JP
        You_hear("a voice say, \"%s, Jim.\"", buf);
*/
        You_hear("「そいつは%s，ジム」という声が聞こえた．", buf);
        *resp = 1;
        return TRUE;

    } else if (corpse) {
        boolean here = (rx == u.ux && ry == u.uy),
                one = (corpse->quan == 1L && !more_corpses), reviver = FALSE;

        if (Role_if(PM_HEALER)) {
            /* ok to reset `corpse' here; we're done with it */
            do {
                if (obj_has_timer(corpse, REVIVE_MON))
                    reviver = TRUE;
                else
                    corpse = nxtobj(corpse, CORPSE, TRUE);
            } while (corpse && !reviver);
        }
#if 0 /*JP*/
        You("determine that %s unfortunate being%s %s%s dead.",
            one ? (here ? "this" : "that") : (here ? "these" : "those"),
            one ? "" : "s", one ? "is" : "are", reviver ? " mostly" : "");
#else
        You("%s不幸な生き物は%s死んでいると結論した．",
            here ? "この" : "その",
            reviver ? "ほぼ" : "");
#endif
        return TRUE;

    } else { /* statue */
        const char *what, *how;

#if 0 /*JP*/
        mptr = &mons[statue->corpsenm];
        if (Blind) { /* ignore statue->dknown; it'll always be set */
            Sprintf(buf, "%s %s",
                    (rx == u.ux && ry == u.uy) ? "This" : "That",
                    humanoid(mptr) ? "person" : "creature");
            what = buf;
        } else {
            what = mptr->mname;
            if (!type_is_pname(mptr))
                what = The(what);
        }
#else /*JP:日本語ではシンプルに*/
        if (Blind) { /* ignore statue->dknown; it'll always be set */
            what = (rx == u.ux && ry == u.uy) ? "これ" : "あれ";
        } else {
            what = mons[statue->corpsenm].mname;
        }
#endif
/*JP
        how = "fine";
*/
        how = "よい";
        if (Role_if(PM_HEALER)) {
            struct trap *ttmp = t_at(rx, ry);

            if (ttmp && ttmp->ttyp == STATUE_TRAP)
/*JP
                how = "extraordinary";
*/
                how = "並外れた";
            else if (Has_contents(statue))
/*JP
                how = "remarkable";
*/
                how = "注目すべき";
        }

/*JP
        pline("%s is in %s health for a statue.", what, how);
*/
        pline("彫像としての%sは%s作品だ．", what, how);
        return TRUE;
    }
    return FALSE; /* no corpse or statue */
}

/*JP
static const char hollow_str[] = "a hollow sound.  This must be a secret %s!";
*/
static const char hollow_str[] = "うつろな音を聞いた．秘密の%sに違いない！";

/* Strictly speaking it makes no sense for usage of a stethoscope to
   not take any time; however, unless it did, the stethoscope would be
   almost useless.  As a compromise, one use per turn is free, another
   uses up the turn; this makes curse status have a tangible effect. */
STATIC_OVL int
use_stethoscope(obj)
register struct obj *obj;
{
    struct monst *mtmp;
    struct rm *lev;
    int rx, ry, res;
    boolean interference = (u.uswallow && is_whirly(u.ustuck->data)
                            && !rn2(Role_if(PM_HEALER) ? 10 : 3));

    if (nohands(youmonst.data)) {
#if 0 /*JP*/
        You("have no hands!"); /* not `body_part(HAND)' */
#else
        pline("あなたには手がない！");
#endif
        return 0;
    } else if (Deaf) {
/*JP
        You_cant("hear anything!");
*/
        You("何も聞こえない！");
        return 0;
    } else if (!freehand()) {
/*JP
        You("have no free %s.", body_part(HAND));
*/
        You("%sの自由が効かない．", body_part(HAND));
        return 0;
    }
    if (!getdir((char *) 0))
        return 0;

    res = (moves == context.stethoscope_move)
          && (youmonst.movement == context.stethoscope_movement);
    context.stethoscope_move = moves;
    context.stethoscope_movement = youmonst.movement;

    if (u.usteed && u.dz > 0) {
        if (interference) {
/*JP
            pline("%s interferes.", Monnam(u.ustuck));
*/
            pline("%sがじゃまをした．", Monnam(u.ustuck));
            mstatusline(u.ustuck);
        } else
            mstatusline(u.usteed);
        return res;
    } else if (u.uswallow && (u.dx || u.dy || u.dz)) {
        mstatusline(u.ustuck);
        return res;
    } else if (u.uswallow && interference) {
/*JP
        pline("%s interferes.", Monnam(u.ustuck));
*/
        pline("%sがじゃまをした．", Monnam(u.ustuck));
        mstatusline(u.ustuck);
        return res;
    } else if (u.dz) {
        if (Underwater)
/*JP
            You_hear("faint splashing.");
*/
            You_hear("かすかにバシャバシャという音を聞いた．");
        else if (u.dz < 0 || !can_reach_floor(TRUE))
            cant_reach_floor(u.ux, u.uy, (u.dz < 0), TRUE);
        else if (its_dead(u.ux, u.uy, &res))
            ; /* message already given */
        else if (Is_stronghold(&u.uz))
/*JP
            You_hear("the crackling of hellfire.");
*/
          You_hear("地獄の炎がパチパチ燃えている音を聞いた．");
        else
/*JP
            pline_The("%s seems healthy enough.", surface(u.ux, u.uy));
*/
            pline("%sは充分健康のようだ．", surface(u.ux,u.uy));
        return res;
    } else if (obj->cursed && !rn2(2)) {
/*JP
        You_hear("your heart beat.");
*/
        You_hear("自分の心臓の鼓動を聞いた．");
        return res;
    }
    if (Stunned || (Confusion && !rn2(5)))
        confdir();
    if (!u.dx && !u.dy) {
        ustatusline();
        return res;
    }
    rx = u.ux + u.dx;
    ry = u.uy + u.dy;
    if (!isok(rx, ry)) {
/*JP
        You_hear("a faint typing noise.");
*/
        You_hear("かすかにだれかがタイピングしている音を聞いた．");
        return 0;
    }
    if ((mtmp = m_at(rx, ry)) != 0) {
        const char *mnm = x_monnam(mtmp, ARTICLE_A, (const char *) 0,
                                   SUPPRESS_IT | SUPPRESS_INVISIBLE, FALSE);

        if (mtmp->mundetected) {
            if (!canspotmon(mtmp))
/*JP
                There("is %s hidden there.", mnm);
*/
                pline("ここに%sが隠れている．", mnm);
            mtmp->mundetected = 0;
            newsym(mtmp->mx, mtmp->my);
        } else if (mtmp->mappearance) {
            const char *what = "thing";

            switch (mtmp->m_ap_type) {
            case M_AP_OBJECT:
                what = simple_typename(mtmp->mappearance);
                break;
            case M_AP_MONSTER: /* ignore Hallucination here */
                what = mons[mtmp->mappearance].mname;
                break;
            case M_AP_FURNITURE:
                what = defsyms[mtmp->mappearance].explanation;
                break;
            }
            seemimic(mtmp);
/*JP
            pline("That %s is really %s", what, mnm);
*/
            pline("この%sは実際には%s．", what, mnm);
        } else if (flags.verbose && !canspotmon(mtmp)) {
/*JP
            There("is %s there.", mnm);
*/
            pline("ここには%sがいる．", mnm);
        }
        mstatusline(mtmp);
        if (!canspotmon(mtmp))
            map_invisible(rx, ry);
        return res;
    }
    if (glyph_is_invisible(levl[rx][ry].glyph)) {
        unmap_object(rx, ry);
        newsym(rx, ry);
/*JP
        pline_The("invisible monster must have moved.");
*/
        pline_The("見えない怪物は移動してしまったようだ．");
    }
    lev = &levl[rx][ry];
    switch (lev->typ) {
    case SDOOR:
/*JP
        You_hear(hollow_str, "door");
*/
        You_hear(hollow_str, "扉");
        cvt_sdoor_to_door(lev); /* ->typ = DOOR */
        feel_newsym(rx, ry);
        return res;
    case SCORR:
/*JP
        You_hear(hollow_str, "passage");
*/
        You_hear(hollow_str, "通路");
        lev->typ = CORR;
        unblock_point(rx, ry);
        feel_newsym(rx, ry);
        return res;
    }

    if (!its_dead(rx, ry, &res))
#if 0 /*JP*/
        You("hear nothing special."); /* not You_hear()  */
#else
        pline("特に何も聞こえない．");
#endif
    return res;
}

/*JP
static const char whistle_str[] = "produce a %s whistling sound.";
*/
static const char whistle_str[] = "笛を吹いて%s音をたてた．";

STATIC_OVL void
use_whistle(obj)
struct obj *obj;
{
    if (!can_blow(&youmonst)) {
/*JP
        You("are incapable of using the whistle.");
*/
        You("笛を使う能力がない．");
    } else if (Underwater) {
/*JP
        You("blow bubbles through %s.", yname(obj));
*/
        You("%sを通して泡を出した．", xname(obj));
    } else {
/*JP
        You(whistle_str, obj->cursed ? "shrill" : "high");
*/
        You(whistle_str, obj->cursed ? "不気味な" : "かん高い");
        wake_nearby();
    }
}

STATIC_OVL void
use_magic_whistle(obj)
struct obj *obj;
{
    register struct monst *mtmp, *nextmon;

    if (!can_blow(&youmonst)) {
/*JP
        You("are incapable of using the whistle.");
*/
        You("笛を使う能力がない．");
    } else if (obj->cursed && !rn2(2)) {
#if 0 /*JP*/
        You("produce a %shigh-pitched humming noise.",
            Underwater ? "very " : "");
#else
        You("%s高い調子のうなるような音をたてた．",
            Underwater ? "とても" : "");
#endif
        wake_nearby();
    } else {
        int pet_cnt = 0, omx, omy;

        /* it's magic!  it works underwater too (at a higher pitch) */
#if 0 /*JP*/
        You(whistle_str,
            Hallucination ? "normal" : Underwater ? "strange, high-pitched"
                                                  : "strange");
#else
        You(whistle_str,
            Hallucination ? "笛のような" : Underwater ? "不思議な高い調子の"
                                                  : "不思議な");
#endif
        for (mtmp = fmon; mtmp; mtmp = nextmon) {
            nextmon = mtmp->nmon; /* trap might kill mon */
            if (DEADMONSTER(mtmp))
                continue;
            /* steed is already at your location, so not affected;
               this avoids trap issues if you're on a trap location */
            if (mtmp == u.usteed)
                continue;
            if (mtmp->mtame) {
                if (mtmp->mtrapped) {
                    /* no longer in previous trap (affects mintrap) */
                    mtmp->mtrapped = 0;
                    fill_pit(mtmp->mx, mtmp->my);
                }
                /* mimic must be revealed before we know whether it
                   actually moves because line-of-sight may change */
                if (mtmp->m_ap_type)
                    seemimic(mtmp);
                omx = mtmp->mx, omy = mtmp->my;
                mnexto(mtmp);
                if (mtmp->mx != omx || mtmp->my != omy) {
                    mtmp->mundetected = 0; /* reveal non-mimic hider */
                    if (canspotmon(mtmp))
                        ++pet_cnt;
                    if (mintrap(mtmp) == 2)
                        change_luck(-1);
                }
            }
        }
        if (pet_cnt > 0)
            makeknown(obj->otyp);
    }
}

boolean
um_dist(x, y, n)
xchar x, y, n;
{
    return (boolean) (abs(u.ux - x) > n || abs(u.uy - y) > n);
}

int
number_leashed()
{
    int i = 0;
    struct obj *obj;

    for (obj = invent; obj; obj = obj->nobj)
        if (obj->otyp == LEASH && obj->leashmon != 0)
            i++;
    return i;
}

/* otmp is about to be destroyed or stolen */
void
o_unleash(otmp)
register struct obj *otmp;
{
    register struct monst *mtmp;

    for (mtmp = fmon; mtmp; mtmp = mtmp->nmon)
        if (mtmp->m_id == (unsigned) otmp->leashmon)
            mtmp->mleashed = 0;
    otmp->leashmon = 0;
}

/* mtmp is about to die, or become untame */
void
m_unleash(mtmp, feedback)
register struct monst *mtmp;
boolean feedback;
{
    register struct obj *otmp;

    if (feedback) {
        if (canseemon(mtmp))
/*JP
            pline("%s pulls free of %s leash!", Monnam(mtmp), mhis(mtmp));
*/
            pline("%sは紐を引っぱって逃れた！", Monnam(mtmp));
        else
/*JP
            Your("leash falls slack.");
*/
            Your("紐がたるんで落ちた．");
    }
    for (otmp = invent; otmp; otmp = otmp->nobj)
        if (otmp->otyp == LEASH && otmp->leashmon == (int) mtmp->m_id)
            otmp->leashmon = 0;
    mtmp->mleashed = 0;
}

/* player is about to die (for bones) */
void
unleash_all()
{
    register struct obj *otmp;
    register struct monst *mtmp;

    for (otmp = invent; otmp; otmp = otmp->nobj)
        if (otmp->otyp == LEASH)
            otmp->leashmon = 0;
    for (mtmp = fmon; mtmp; mtmp = mtmp->nmon)
        mtmp->mleashed = 0;
}

#define MAXLEASHED 2

static boolean
leashable(mtmp)
struct monst *mtmp;
{
    return (boolean) (mtmp->mnum != PM_LONG_WORM);
}

/* ARGSUSED */
STATIC_OVL void
use_leash(obj)
struct obj *obj;
{
    coord cc;
    register struct monst *mtmp;
    int spotmon;

    if (!obj->leashmon && number_leashed() >= MAXLEASHED) {
/*JP
        You("cannot leash any more pets.");
*/
        You("これ以上ペットに紐をかけられない．");
        return;
    }

    if (!get_adjacent_loc((char *) 0, (char *) 0, u.ux, u.uy, &cc))
        return;

    if ((cc.x == u.ux) && (cc.y == u.uy)) {
        if (u.usteed && u.dz > 0) {
            mtmp = u.usteed;
            spotmon = 1;
            goto got_target;
        }
/*JP
        pline("Leash yourself?  Very funny...");
*/
        pline("自分を縛る？変なの．．．");
        return;
    }

    if (!(mtmp = m_at(cc.x, cc.y))) {
/*JP
        There("is no creature there.");
*/
        pline("そこには生き物はいない．");
        return;
    }

    spotmon = canspotmon(mtmp);
got_target:

    if (!mtmp->mtame) {
        if (!spotmon)
/*JP
            There("is no creature there.");
*/
            pline("そこには生き物はいない．");
        else
#if 0 /*JP*/
            pline("%s %s leashed!", Monnam(mtmp),
                  (!obj->leashmon) ? "cannot be" : "is not");
#else
          pline("%sは紐で%s！", Monnam(mtmp),
                  (!obj->leashmon) ? "結べない" : "結ばれていない");
#endif
        return;
    }
    if (!obj->leashmon) {
        if (mtmp->mleashed) {
#if 0 /*JP*/
            pline("This %s is already leashed.",
                  spotmon ? l_monnam(mtmp) : "monster");
#else
            pline("%sはすでに結びつけられている．",
                  spotmon ? l_monnam(mtmp) : "怪物");
#endif
            return;
        }
        if (!leashable(mtmp)) {
#if 0 /*JP*/
            pline("The leash won't fit onto %s%s.", spotmon ? "your " : "",
                  l_monnam(mtmp));
#else
            pline("紐は%sに合わない．",
                  l_monnam(mtmp));
#endif
            return;
        }

#if 0 /*JP*/
        You("slip the leash around %s%s.", spotmon ? "your " : "",
            l_monnam(mtmp));
#else
        You("%sを紐で結びつけた．",
            l_monnam(mtmp));
#endif
        mtmp->mleashed = 1;
        obj->leashmon = (int) mtmp->m_id;
        mtmp->msleeping = 0;
        return;
    }
    if (obj->leashmon != (int) mtmp->m_id) {
/*JP
        pline("This leash is not attached to that creature.");
*/
        pline("この紐はそれには結ばれていない．");
        return;
    } else {
        if (obj->cursed) {
/*JP
            pline_The("leash would not come off!");
*/
            pline("紐がはずれない！");
            obj->bknown = TRUE;
            return;
        }
        mtmp->mleashed = 0;
        obj->leashmon = 0;
#if 0 /*JP*/
        You("remove the leash from %s%s.", spotmon ? "your " : "",
            l_monnam(mtmp));
#else
        You("%sから紐をはずした．",
            l_monnam(mtmp));
#endif
    }
    return;
}

/* assuming mtmp->mleashed has been checked */
struct obj *
get_mleash(mtmp)
struct monst *mtmp;
{
    struct obj *otmp;

    otmp = invent;
    while (otmp) {
        if (otmp->otyp == LEASH && otmp->leashmon == (int) mtmp->m_id)
            return otmp;
        otmp = otmp->nobj;
    }
    return (struct obj *) 0;
}

boolean
next_to_u()
{
    register struct monst *mtmp;
    register struct obj *otmp;

    for (mtmp = fmon; mtmp; mtmp = mtmp->nmon) {
        if (DEADMONSTER(mtmp))
            continue;
        if (mtmp->mleashed) {
            if (distu(mtmp->mx, mtmp->my) > 2)
                mnexto(mtmp);
            if (distu(mtmp->mx, mtmp->my) > 2) {
                for (otmp = invent; otmp; otmp = otmp->nobj)
                    if (otmp->otyp == LEASH
                        && otmp->leashmon == (int) mtmp->m_id) {
                        if (otmp->cursed)
                            return FALSE;
#if 0 /*JP*/
                        You_feel("%s leash go slack.",
                                 (number_leashed() > 1) ? "a" : "the");
#else
                        You("紐がたるんだような気がした．");
#endif
                        mtmp->mleashed = 0;
                        otmp->leashmon = 0;
                    }
            }
        }
    }
    /* no pack mules for the Amulet */
    if (u.usteed && mon_has_amulet(u.usteed))
        return FALSE;
    return TRUE;
}

void
check_leash(x, y)
register xchar x, y;
{
    register struct obj *otmp;
    register struct monst *mtmp;

    for (otmp = invent; otmp; otmp = otmp->nobj) {
        if (otmp->otyp != LEASH || otmp->leashmon == 0)
            continue;
        for (mtmp = fmon; mtmp; mtmp = mtmp->nmon) {
            if (DEADMONSTER(mtmp))
                continue;
            if ((int) mtmp->m_id == otmp->leashmon)
                break;
        }
        if (!mtmp) {
            impossible("leash in use isn't attached to anything?");
            otmp->leashmon = 0;
            continue;
        }
        if (dist2(u.ux, u.uy, mtmp->mx, mtmp->my)
            > dist2(x, y, mtmp->mx, mtmp->my)) {
            if (!um_dist(mtmp->mx, mtmp->my, 3)) {
                ; /* still close enough */
            } else if (otmp->cursed && !breathless(mtmp->data)) {
                if (um_dist(mtmp->mx, mtmp->my, 5)
                    || (mtmp->mhp -= rnd(2)) <= 0) {
                    long save_pacifism = u.uconduct.killer;

/*JP
                    Your("leash chokes %s to death!", mon_nam(mtmp));
*/
                    pline("%sは絞め殺された！",mon_nam(mtmp));
                    /* hero might not have intended to kill pet, but
                       that's the result of his actions; gain experience,
                       lose pacifism, take alignment and luck hit, make
                       corpse less likely to remain tame after revival */
                    xkilled(mtmp, 0); /* no "you kill it" message */
                    /* life-saving doesn't ordinarily reset this */
                    if (mtmp->mhp > 0)
                        u.uconduct.killer = save_pacifism;
                } else {
/*JP
                    pline("%s is choked by the leash!", Monnam(mtmp));
*/
                    pline("%sは紐で首を絞められた！", Monnam(mtmp));
                    /* tameness eventually drops to 1 here (never 0) */
                    if (mtmp->mtame && rn2(mtmp->mtame))
                        mtmp->mtame--;
                }
            } else {
                if (um_dist(mtmp->mx, mtmp->my, 5)) {
/*JP
                    pline("%s leash snaps loose!", s_suffix(Monnam(mtmp)));
*/
                    pline("%sの紐はパチンと外れた！", Monnam(mtmp));
                    m_unleash(mtmp, FALSE);
                } else {
/*JP
                    You("pull on the leash.");
*/
                    You("紐を引っぱった．");
                    if (mtmp->data->msound != MS_SILENT)
                        switch (rn2(3)) {
                        case 0:
                            growl(mtmp);
                            break;
                        case 1:
                            yelp(mtmp);
                            break;
                        default:
                            whimper(mtmp);
                            break;
                        }
                }
            }
        }
    }
}

const char *
beautiful()
{
    return ((ACURR(A_CHA) > 14)
               ? ((poly_gender() == 1)
#if 0 /*JP*/
                     ? "beautiful"
                     : "handsome")
               : "ugly");
#else
/*JP 語幹で使う */
                     ? "美し"
                     : "りりし")
               : "醜");
#endif
}

#define WEAK 3 /* from eat.c */

/*JP
static const char look_str[] = "look %s.";
*/
static const char look_str[] = "%s見える．";

STATIC_OVL int
use_mirror(obj)
struct obj *obj;
{
    const char *mirror, *uvisage;
    struct monst *mtmp;
    unsigned how_seen;
    char mlet;
    boolean vis, invis_mirror, useeit, monable;

    if (!getdir((char *) 0))
        return 0;
    invis_mirror = Invis;
    useeit = !Blind && (!invis_mirror || See_invisible);
    uvisage = beautiful();
    mirror = simpleonames(obj); /* "mirror" or "looking glass" */
    if (obj->cursed && !rn2(2)) {
        if (!Blind)
/*JP
            pline_The("%s fogs up and doesn't reflect!", mirror);
*/
            pline("%sは曇り，映らなくなった！", mirror);
        return 1;
    }
    if (!u.dx && !u.dy && !u.dz) {
        if (!useeit) {
/*JP
            You_cant("see your %s %s.", uvisage, body_part(FACE));
*/
            You_cant("自分の%sい%sを見ることができない．", uvisage, body_part(FACE));
        } else {
            if (u.umonnum == PM_FLOATING_EYE) {
                if (Free_action) {
/*JP
                    You("stiffen momentarily under your gaze.");
*/
                    pline("一瞬あなたのにらみで硬直した．");
                } else {
                    if (Hallucination)
/*JP
                        pline("Yow!  The %s stares back!", mirror);
*/
                        pline("おゎ！%sがあなたをにらみ返した！", mirror);
                    else
/*JP
                        pline("Yikes!  You've frozen yourself!");
*/
                        pline("おゎ！あなたは動けなくなった！");
                    if (!Hallucination || !rn2(4)) {
                        nomul(-rnd(MAXULEV + 6 - u.ulevel));
                        multi_reason = "gazing into a mirror";
                    }
                    nomovemsg = 0; /* default, "you can move again" */
                }
            } else if (youmonst.data->mlet == S_VAMPIRE)
/*JP
                You("don't have a reflection.");
*/
                You("鏡に映らなかった．");
            else if (u.umonnum == PM_UMBER_HULK) {
/*JP
                pline("Huh?  That doesn't look like you!");
*/
                pline("ほえ？写ってるのはあなたじゃないみたいだ！");
                make_confused(HConfusion + d(3, 4), FALSE);
            } else if (Hallucination)
                You(look_str, hcolor((char *) 0));
            else if (Sick)
/*JP
                You(look_str, "peaked");
*/
                You(look_str, "顔色が悪く");
            else if (u.uhs >= WEAK)
/*JP
                You(look_str, "undernourished");
*/
                You(look_str, "栄養失調のように");
            else
/*JP
                You("look as %s as ever.", uvisage);
*/
                You("あいかわらず%sく見える．", uvisage);
        }
        return 1;
    }
    if (u.uswallow) {
        if (useeit)
            You("reflect %s %s.", s_suffix(mon_nam(u.ustuck)),
                mbodypart(u.ustuck, STOMACH));
        return 1;
    }
    if (Underwater) {
        if (useeit)
#if 0 /*JP*/
            You(Hallucination ? "give the fish a chance to fix their makeup."
                              : "reflect the murky water.");
#else
            You(Hallucination ? "魚に化粧直しの機会を与えた．"
                              : "あなたは淀んだ水を映した．");
#endif
        return 1;
    }
    if (u.dz) {
        if (useeit)
/*JP
            You("reflect the %s.",
*/
            You("%sを映した．",
                (u.dz > 0) ? surface(u.ux, u.uy) : ceiling(u.ux, u.uy));
        return 1;
    }
    mtmp = bhit(u.dx, u.dy, COLNO, INVIS_BEAM,
                (int FDECL((*), (MONST_P, OBJ_P))) 0,
                (int FDECL((*), (OBJ_P, OBJ_P))) 0, &obj);
    if (!mtmp || !haseyes(mtmp->data) || notonhead)
        return 1;

    /* couldsee(mtmp->mx, mtmp->my) is implied by the fact that bhit()
       targetted it, so we can ignore possibility of X-ray vision */
    vis = canseemon(mtmp);
/* ways to directly see monster (excludes X-ray vision, telepathy,
   extended detection, type-specific warning) */
#define SEENMON (MONSEEN_NORMAL | MONSEEN_SEEINVIS | MONSEEN_INFRAVIS)
    how_seen = vis ? howmonseen(mtmp) : 0;
    /* whether monster is able to use its vision-based capabilities */
    monable = !mtmp->mcan && (!mtmp->minvis || perceives(mtmp->data));
    mlet = mtmp->data->mlet;
    if (mtmp->msleeping) {
        if (vis)
/*JP
            pline("%s is too tired to look at your %s.", Monnam(mtmp),
*/
            pline("%sはとても疲れていて%sを見るどころじゃない．", Monnam(mtmp),
                  mirror);
    } else if (!mtmp->mcansee) {
        if (vis)
/*JP
            pline("%s can't see anything right now.", Monnam(mtmp));
*/
            pline("%sは今のところ何も見ることができない．", Monnam(mtmp));
    } else if (invis_mirror && !perceives(mtmp->data)) {
        if (vis)
/*JP
            pline("%s fails to notice your %s.", Monnam(mtmp), mirror);
*/
            pline("%sはあなたの%sに気付かなかった．", Monnam(mtmp), mirror);
        /* infravision doesn't produce an image in the mirror */
    } else if ((how_seen & SEENMON) == MONSEEN_INFRAVIS) {
        if (vis) /* (redundant) */
#if 0 /*JP*/
            pline("%s is too far away to see %sself in the dark.",
                  Monnam(mtmp), mhim(mtmp));
#else
            pline("%sは暗闇で自分自身を見るには遠すぎる．", Monnam(mtmp));
#endif
        /* some monsters do special things */
    } else if (mlet == S_VAMPIRE || mlet == S_GHOST || is_vampshifter(mtmp)) {
        if (vis)
/*JP
            pline("%s doesn't have a reflection.", Monnam(mtmp));
*/
            pline("%sは%sに映らない．", Monnam(mtmp), mirror);
    } else if (monable && mtmp->data == &mons[PM_MEDUSA]) {
/*JP
        if (mon_reflects(mtmp, "The gaze is reflected away by %s %s!"))
*/
        if (mon_reflects(mtmp, "にらみは%sの%sで反射した！"))
            return 1;
        if (vis)
/*JP
            pline("%s is turned to stone!", Monnam(mtmp));
*/
            pline("%sは石になった！", Monnam(mtmp));
        stoned = TRUE;
        killed(mtmp);
    } else if (monable && mtmp->data == &mons[PM_FLOATING_EYE]) {
        int tmp = d((int) mtmp->m_lev, (int) mtmp->data->mattk[0].damd);
        if (!rn2(4))
            tmp = 120;
        if (vis)
/*JP
            pline("%s is frozen by its reflection.", Monnam(mtmp));
*/
            pline("%sは自分の姿を見て動けなくなった．", Monnam(mtmp));
        else
/*JP
            You_hear("%s stop moving.", something);
*/
            You_hear("何かが動きをとめた音を聞いた．");
        paralyze_monst(mtmp, (int) mtmp->mfrozen + tmp);
    } else if (monable && mtmp->data == &mons[PM_UMBER_HULK]) {
        if (vis)
/*JP
            pline("%s confuses itself!", Monnam(mtmp));
*/
            pline("%sは混乱した！", Monnam(mtmp));
        mtmp->mconf = 1;
    } else if (monable && (mlet == S_NYMPH || mtmp->data == &mons[PM_SUCCUBUS]
                           || mtmp->data == &mons[PM_INCUBUS])) {
        if (vis) {
            char buf[BUFSZ]; /* "She" or "He" */

#if 0 /*JP*/
            pline("%s admires %sself in your %s.", Monnam(mtmp), mhim(mtmp),
                  mirror);
#else
            pline("%sは自分の姿にうっとりした．", Monnam(mtmp));
#endif
/*JP
            pline("%s takes it!", upstart(strcpy(buf, mhe(mtmp))));
*/
            pline("%sはそれを奪った！", upstart(strcpy(buf, mhe(mtmp))));
        } else
/*JP
            pline("It steals your %s!", mirror);
*/
            pline("何者かがあなたの%sを盗んだ！", mirror);
        setnotworn(obj); /* in case mirror was wielded */
        freeinv(obj);
        (void) mpickobj(mtmp, obj);
        if (!tele_restrict(mtmp))
            (void) rloc(mtmp, TRUE);
    } else if (!is_unicorn(mtmp->data) && !humanoid(mtmp->data)
               && (!mtmp->minvis || perceives(mtmp->data)) && rn2(5)) {
        if (vis)
/*JP
            pline("%s is frightened by its reflection.", Monnam(mtmp));
*/
            pline("%sは自分の姿を見て怖がった．", Monnam(mtmp));
        monflee(mtmp, d(2, 4), FALSE, FALSE);
    } else if (!Blind) {
        if (mtmp->minvis && !See_invisible)
            ;
        else if ((mtmp->minvis && !perceives(mtmp->data))
                 /* redundant: can't get here if these are true */
                 || !haseyes(mtmp->data) || notonhead || !mtmp->mcansee)
#if 0 /*JP*/
            pline("%s doesn't seem to notice %s reflection.", Monnam(mtmp),
                  mhis(mtmp));
#else
            pline("%sは自分の姿に気がついてないようだ．", Monnam(mtmp));
#endif
        else
/*JP
            pline("%s ignores %s reflection.", Monnam(mtmp), mhis(mtmp));
*/
            pline("%sは自分の姿を無視した．", Monnam(mtmp));
    }
    return 1;
}

STATIC_OVL void
use_bell(optr)
struct obj **optr;
{
    register struct obj *obj = *optr;
    struct monst *mtmp;
    boolean wakem = FALSE, learno = FALSE,
            ordinary = (obj->otyp != BELL_OF_OPENING || !obj->spe),
            invoking =
                (obj->otyp == BELL_OF_OPENING && invocation_pos(u.ux, u.uy)
                 && !On_stairs(u.ux, u.uy));

/*JP
    You("ring %s.", the(xname(obj)));
*/
    You("%sを鳴らした．", the(xname(obj)));

    if (Underwater || (u.uswallow && ordinary)) {
#ifdef AMIGA
        amii_speaker(obj, "AhDhGqEqDhEhAqDqFhGw", AMII_MUFFLED_VOLUME);
#endif
/*JP
        pline("But the sound is muffled.");
*/
        pline("しかし音はかき消された．");

    } else if (invoking && ordinary) {
        /* needs to be recharged... */
/*JP
        pline("But it makes no sound.");
*/
        pline("しかし，音は鳴らなかった．");
        learno = TRUE; /* help player figure out why */

    } else if (ordinary) {
#ifdef AMIGA
        amii_speaker(obj, "ahdhgqeqdhehaqdqfhgw", AMII_MUFFLED_VOLUME);
#endif
        if (obj->cursed && !rn2(4)
            /* note: once any of them are gone, we stop all of them */
            && !(mvitals[PM_WOOD_NYMPH].mvflags & G_GONE)
            && !(mvitals[PM_WATER_NYMPH].mvflags & G_GONE)
            && !(mvitals[PM_MOUNTAIN_NYMPH].mvflags & G_GONE)
            && (mtmp = makemon(mkclass(S_NYMPH, 0), u.ux, u.uy, NO_MINVENT))
                   != 0) {
/*JP
            You("summon %s!", a_monnam(mtmp));
*/
            You("%sを召喚した！", a_monnam(mtmp));
            if (!obj_resists(obj, 93, 100)) {
/*JP
                pline("%s shattered!", Tobjnam(obj, "have"));
*/
                pline("%sは粉々になった！", xname(obj));
                useup(obj);
                *optr = 0;
            } else
                switch (rn2(3)) {
                default:
                    break;
                case 1:
                    mon_adjust_speed(mtmp, 2, (struct obj *) 0);
                    break;
                case 2: /* no explanation; it just happens... */
                    nomovemsg = "";
                    multi_reason = NULL;
                    nomul(-rnd(2));
                    break;
                }
        }
        wakem = TRUE;

    } else {
        /* charged Bell of Opening */
        consume_obj_charge(obj, TRUE);

        if (u.uswallow) {
            if (!obj->cursed)
                (void) openit();
            else
                pline1(nothing_happens);

        } else if (obj->cursed) {
            coord mm;

            mm.x = u.ux;
            mm.y = u.uy;
            mkundead(&mm, FALSE, NO_MINVENT);
            wakem = TRUE;

        } else if (invoking) {
/*JP
            pline("%s an unsettling shrill sound...", Tobjnam(obj, "issue"));
*/
            pline("%sは不気味な鋭い音を出した．．．", xname(obj));
#ifdef AMIGA
            amii_speaker(obj, "aefeaefeaefeaefeaefe", AMII_LOUDER_VOLUME);
#endif
            obj->age = moves;
            learno = TRUE;
            wakem = TRUE;

        } else if (obj->blessed) {
            int res = 0;

#ifdef AMIGA
            amii_speaker(obj, "ahahahDhEhCw", AMII_SOFT_VOLUME);
#endif
            if (uchain) {
                unpunish();
                res = 1;
            } else if (u.utrap && u.utraptype == TT_BURIEDBALL) {
                buried_ball_to_freedom();
                res = 1;
            }
            res += openit();
            switch (res) {
            case 0:
                pline1(nothing_happens);
                break;
            case 1:
/*JP
                pline("%s opens...", Something);
*/
                pline("何かが開いた．．．");
                learno = TRUE;
                break;
            default:
/*JP
                pline("Things open around you...");
*/
                pline("まわりの物が開いた．．．");
                learno = TRUE;
                break;
            }

        } else { /* uncursed */
#ifdef AMIGA
            amii_speaker(obj, "AeFeaeFeAefegw", AMII_OKAY_VOLUME);
#endif
            if (findit() != 0)
                learno = TRUE;
            else
                pline1(nothing_happens);
        }

    } /* charged BofO */

    if (learno) {
        makeknown(BELL_OF_OPENING);
        obj->known = 1;
    }
    if (wakem)
        wake_nearby();
}

STATIC_OVL void
use_candelabrum(obj)
register struct obj *obj;
{
#if 0 /*JP*//* not used */
    const char *s = (obj->spe != 1) ? "candles" : "candle";
#endif

    if (obj->lamplit) {
/*JP
        You("snuff the %s.", s);
*/
        You("ろうそくを吹き消した．");
        end_burn(obj, TRUE);
        return;
    }
    if (obj->spe <= 0) {
/*JP
        pline("This %s has no %s.", xname(obj), s);
*/
        pline("この%sにはろうそくがない．", xname(obj));
        return;
    }
    if (Underwater) {
/*JP
        You("cannot make fire under water.");
*/
        You("水中で火をおこせない．");
        return;
    }
    if (u.uswallow || obj->cursed) {
        if (!Blind)
#if 0 /*JP*/
            pline_The("%s %s for a moment, then %s.", s, vtense(s, "flicker"),
                      vtense(s, "die"));
#else
            pline("ろうそくの炎はしばらく点滅し，消えた．");
#endif
        return;
    }
    if (obj->spe < 7) {
#if 0 /*JP*/
        There("%s only %d %s in %s.", vtense(s, "are"), obj->spe, s,
              the(xname(obj)));
#else
        pline("%sにはたった%d本のろうそくしかない．",
              xname(obj), obj->spe);
#endif
        if (!Blind)
#if 0 /*JP*/
            pline("%s lit.  %s dimly.", obj->spe == 1 ? "It is" : "They are",
                  Tobjnam(obj, "shine"));
#else
            pline("%sに火をつけた．%sはほのかに輝いた．",
                  xname(obj), xname(obj));
#endif
    } else {
#if 0 /*JP*/
        pline("%s's %s burn%s", The(xname(obj)), s,
              (Blind ? "." : " brightly!"));
#else
        pline("%sのろうそくは%s燃えあがった！", The(xname(obj)),
              (Blind ? "" : "明るく"));
#endif
    }
    if (!invocation_pos(u.ux, u.uy) || On_stairs(u.ux, u.uy)) {
/*JP
        pline_The("%s %s being rapidly consumed!", s, vtense(s, "are"));
*/
        pline("ろうそくはすごい速さで燃え始めた！");
        /* this used to be obj->age /= 2, rounding down; an age of
           1 would yield 0, confusing begin_burn() and producing an
           unlightable, unrefillable candelabrum; round up instead */
        obj->age = (obj->age + 1L) / 2L;
    } else {
        if (obj->spe == 7) {
            if (Blind)
/*JP
                pline("%s a strange warmth!", Tobjnam(obj, "radiate"));
*/
                pline("奇妙な暖かさを%sに感じた！", xname(obj));
            else
/*JP
                pline("%s with a strange light!", Tobjnam(obj, "glow"));
*/
                pline("%sは奇妙な光を発している！", xname(obj));
        }
        obj->known = 1;
    }
    begin_burn(obj, FALSE);
}

STATIC_OVL void
use_candle(optr)
struct obj **optr;
{
    register struct obj *obj = *optr;
    register struct obj *otmp;
/*JP
    const char *s = (obj->quan != 1) ? "candles" : "candle";
*/
    const char *s = "ろうそく";
    char qbuf[QBUFSZ], qsfx[QBUFSZ], *q;

    if (u.uswallow) {
        You(no_elbow_room);
        return;
    }

    otmp = carrying(CANDELABRUM_OF_INVOCATION);
    if (!otmp || otmp->spe == 7) {
        use_lamp(obj);
        return;
    }

    /*JP:最終的には「ろうそくを燭台に取りつけますか？」*/
    /* first, minimal candelabrum suffix for formatting candles */
/*JP
    Sprintf(qsfx, " to\033%s?", thesimpleoname(otmp));
*/
    Sprintf(qsfx, "を\033%sに取りつけますか？", thesimpleoname(otmp));
    /* next, format the candles as a prefix for the candelabrum */
/*JP
    (void) safe_qbuf(qbuf, "Attach ", qsfx, obj, yname, thesimpleoname, s);
*/
    (void) safe_qbuf(qbuf, "", qsfx, obj, xname, thesimpleoname, s);
    /*JP:「(ろうそく)を\033燭台に取りつけますか？」*/
    /* strip temporary candelabrum suffix */
#if 0 /*JP*/
    if ((q = strstri(qbuf, " to\033")) != 0)
        Strcpy(q, " to ");
#else
    if ((q = strchr(qbuf, '\033')) != 0)
        *q = '\0';
    /*JP:「(ろうそく)を」*/
#endif
    /* last, format final "attach candles to candelabrum?" query */
/*JP
    if (yn(safe_qbuf(qbuf, qbuf, "?", otmp, yname, thesimpleoname, "it"))
*/
    if (yn(safe_qbuf(qbuf, qbuf, "に取りつけますか？", otmp, xname, thesimpleoname, "それ"))
        == 'n') {
        use_lamp(obj);
        return;
    } else {
        if ((long) otmp->spe + obj->quan > 7L) {
            obj = splitobj(obj, 7L - (long) otmp->spe);
#if 0 /*JP:日本語では不要*/
            /* avoid a grammatical error if obj->quan gets
               reduced to 1 candle from more than one */
            s = (obj->quan != 1) ? "candles" : "candle";
#endif
        } else
            *optr = 0;
#if 0 /*JP*/
        You("attach %ld%s %s to %s.", obj->quan, !otmp->spe ? "" : " more", s,
            the(xname(otmp)));
#else
        You("%ld本のろうそくを%s%sへ取りつけた．",
            obj->quan, !otmp->spe ? "" : "さらに",
            xname(otmp));
#endif
        if (!otmp->spe || otmp->age > obj->age)
            otmp->age = obj->age;
        otmp->spe += (int) obj->quan;
        if (otmp->lamplit && !obj->lamplit)
/*JP
            pline_The("new %s magically %s!", s, vtense(s, "ignite"));
*/
            pline("新しいろうそくは不思議な炎をあげた！");
        else if (!otmp->lamplit && obj->lamplit)
/*JP
            pline("%s out.", (obj->quan > 1L) ? "They go" : "It goes");
*/
            pline("炎は消えた．");
        if (obj->unpaid)
#if 0 /*JP*/
            verbalize("You %s %s, you bought %s!",
                      otmp->lamplit ? "burn" : "use",
                      (obj->quan > 1L) ? "them" : "it",
                      (obj->quan > 1L) ? "them" : "it");
#else
            verbalize("火をつけたなら，買ってもらおう！");
#endif
        if (obj->quan < 7L && otmp->spe == 7)
#if 0 /*JP*/
            pline("%s now has seven%s candles attached.", The(xname(otmp)),
                  otmp->lamplit ? " lit" : "");
#else
            pline("%sにはすでに7本の%sろうそくが取りつけられている．",
                  The(xname(otmp)), otmp->lamplit ? "火のついた" : "");
#endif
        /* candelabrum's light range might increase */
        if (otmp->lamplit)
            obj_merge_light_sources(otmp, otmp);
        /* candles are no longer a separate light source */
        if (obj->lamplit)
            end_burn(obj, TRUE);
        /* candles are now gone */
        useupall(obj);
    }
}

/* call in drop, throw, and put in box, etc. */
boolean
snuff_candle(otmp)
struct obj *otmp;
{
    boolean candle = Is_candle(otmp);

    if ((candle || otmp->otyp == CANDELABRUM_OF_INVOCATION)
        && otmp->lamplit) {
        char buf[BUFSZ];
        xchar x, y;
#if 0 /*JP*//* not used */
        boolean many = candle ? (otmp->quan > 1L) : (otmp->spe > 1);
#endif

        (void) get_obj_location(otmp, &x, &y, 0);
        if (otmp->where == OBJ_MINVENT ? cansee(x, y) : !Blind)
#if 0 /*JP*/
            pline("%s%scandle%s flame%s extinguished.", Shk_Your(buf, otmp),
                  (candle ? "" : "candelabrum's "), (many ? "s'" : "'s"),
                  (many ? "s are" : " is"));
#else
            pline("%s%sろうそくの炎は消えた．", Shk_Your(buf, otmp),
                  candle ? "" : "燭台の");
#endif
        end_burn(otmp, TRUE);
        return TRUE;
    }
    return FALSE;
}

/* called when lit lamp is hit by water or put into a container or
   you've been swallowed by a monster; obj might be in transit while
   being thrown or dropped so don't assume that its location is valid */
boolean
snuff_lit(obj)
struct obj *obj;
{
    xchar x, y;

    if (obj->lamplit) {
        if (obj->otyp == OIL_LAMP || obj->otyp == MAGIC_LAMP
            || obj->otyp == BRASS_LANTERN || obj->otyp == POT_OIL) {
            (void) get_obj_location(obj, &x, &y, 0);
            if (obj->where == OBJ_MINVENT ? cansee(x, y) : !Blind)
/*JP
                pline("%s %s out!", Yname2(obj), otense(obj, "go"));
*/
                pline("%sは消えた！", Yname2(obj));
            end_burn(obj, TRUE);
            return TRUE;
        }
        if (snuff_candle(obj))
            return TRUE;
    }
    return FALSE;
}

/* Called when potentially lightable object is affected by fire_damage().
   Return TRUE if object was lit and FALSE otherwise --ALI */
boolean
catch_lit(obj)
struct obj *obj;
{
    xchar x, y;

    if (!obj->lamplit && (obj->otyp == MAGIC_LAMP || ignitable(obj))) {
        if ((obj->otyp == MAGIC_LAMP
             || obj->otyp == CANDELABRUM_OF_INVOCATION) && obj->spe == 0)
            return FALSE;
        else if (obj->otyp != MAGIC_LAMP && obj->age == 0)
            return FALSE;
        if (!get_obj_location(obj, &x, &y, 0))
            return FALSE;
        if (obj->otyp == CANDELABRUM_OF_INVOCATION && obj->cursed)
            return FALSE;
        if ((obj->otyp == OIL_LAMP || obj->otyp == MAGIC_LAMP
             || obj->otyp == BRASS_LANTERN) && obj->cursed && !rn2(2))
            return FALSE;
        if (obj->where == OBJ_MINVENT ? cansee(x, y) : !Blind)
/*JP
            pline("%s %s light!", Yname2(obj), otense(obj, "catch"));
*/
            pline("%sの明かりがついた！", Yname2(obj));
        if (obj->otyp == POT_OIL)
            makeknown(obj->otyp);
        if (carried(obj) && obj->unpaid && costly_spot(u.ux, u.uy)) {
            /* if it catches while you have it, then it's your tough luck */
            check_unpaid(obj);
#if 0 /*JP:T*/
            verbalize("That's in addition to the cost of %s %s, of course.",
                      yname(obj), obj->quan == 1L ? "itself" : "themselves");
#else
            verbalize("これはもちろん%sの値段とは別だよ．", xname(obj));
#endif
            bill_dummy_object(obj);
        }
        begin_burn(obj, FALSE);
        return TRUE;
    }
    return FALSE;
}

STATIC_OVL void
use_lamp(obj)
struct obj *obj;
{
    char buf[BUFSZ];

    if (obj->lamplit) {
        if (obj->otyp == OIL_LAMP || obj->otyp == MAGIC_LAMP
            || obj->otyp == BRASS_LANTERN)
/*JP
            pline("%slamp is now off.", Shk_Your(buf, obj));
*/
	    pline("%sランプの灯は消えた．", Shk_Your(buf, obj));
        else
/*JP
            You("snuff out %s.", yname(obj));
*/
            You("%sを吹き消した．", xname(obj));
        end_burn(obj, TRUE);
        return;
    }
    if (Underwater) {
#if 0 /*JP*/
        pline(!Is_candle(obj) ? "This is not a diving lamp"
                              : "Sorry, fire and water don't mix.");
#else
        pline(!Is_candle(obj) ? "これは潜水用のランプじゃない．"
                              : "残念ながら，火と水はまざらない．");
#endif
        return;
    }
    /* magic lamps with an spe == 0 (wished for) cannot be lit */
    if ((!Is_candle(obj) && obj->age == 0)
        || (obj->otyp == MAGIC_LAMP && obj->spe == 0)) {
        if (obj->otyp == BRASS_LANTERN)
/*JP
            Your("lamp has run out of power.");
*/
            Your("ランプの電力を使い切ってしまった．");
        else
/*JP
            pline("This %s has no oil.", xname(obj));
*/
            pline("この%sにはもうオイルがない．", xname(obj));
        return;
    }
    if (obj->cursed && !rn2(2)) {
        if (!Blind)
#if 0 /*JP*/
            pline("%s for a moment, then %s.", Tobjnam(obj, "flicker"),
                  otense(obj, "die"));
#else
            pline("%sはしばらくの間点滅し，消えた．",
                  xname(obj));
#endif
    } else {
        if (obj->otyp == OIL_LAMP || obj->otyp == MAGIC_LAMP
            || obj->otyp == BRASS_LANTERN) {
            check_unpaid(obj);
/*JP
            pline("%slamp is now on.", Shk_Your(buf, obj));
*/
            pline("%sランプに灯が灯った．", Shk_Your(buf, obj));
        } else { /* candle(s) */
#if 0 /*JP*/
            pline("%s flame%s %s%s", s_suffix(Yname2(obj)), plur(obj->quan),
                  otense(obj, "burn"), Blind ? "." : " brightly!");
#else
            pline("%sは%s燃えあがった！",
                  xname(obj), Blind ? "" : "明るく");
#endif
            if (obj->unpaid && costly_spot(u.ux, u.uy)
                && obj->age == 20L * (long) objects[obj->otyp].oc_cost) {
#if 0 /*JP*/
                const char *ithem = (obj->quan > 1L) ? "them" : "it";

                verbalize("You burn %s, you bought %s!", ithem, ithem);
#else
                verbalize("灯をつけたなら，買ってもらおう！");
#endif
                bill_dummy_object(obj);
            }
        }
        begin_burn(obj, FALSE);
    }
}

STATIC_OVL void
light_cocktail(obj)
struct obj *obj; /* obj is a potion of oil */
{
    char buf[BUFSZ];
    boolean split1off;

    if (u.uswallow) {
        You(no_elbow_room);
        return;
    }

    if (obj->lamplit) {
/*JP
        You("snuff the lit potion.");
*/
        You("油瓶の火を吹き消した．");
        end_burn(obj, TRUE);
        /*
         * Free & add to re-merge potion.  This will average the
         * age of the potions.  Not exactly the best solution,
         * but its easy.
         */
        freeinv(obj);
        (void) addinv(obj);
        return;
    } else if (Underwater) {
/*JP
        There("is not enough oxygen to sustain a fire.");
*/
        pline("火をつけるのに十分な酸素がない．");
        return;
    }

    split1off = (obj->quan > 1L);
    if (split1off)
        obj = splitobj(obj, 1L);

#if 0 /*JP*/
    You("light %spotion.%s", shk_your(buf, obj),
        Blind ? "" : "  It gives off a dim light.");
#else
    You("%s油瓶に火をつけた．%s", shk_your(buf, obj),
        Blind ? "" : "油瓶は暗い光をはなった．");
#endif

    if (obj->unpaid && costly_spot(u.ux, u.uy)) {
        /* Normally, we shouldn't both partially and fully charge
         * for an item, but (Yendorian Fuel) Taxes are inevitable...
         */
        check_unpaid(obj);
/*JP
        verbalize("That's in addition to the cost of the potion, of course.");
*/
        verbalize("これはもちろん油瓶の値段とは別だよ．");
        bill_dummy_object(obj);
    }
    makeknown(obj->otyp);

    begin_burn(obj, FALSE); /* after shop billing */
    if (split1off) {
        obj_extract_self(obj); /* free from inv */
        obj->nomerge = 1;
/*JP
        obj = hold_another_object(obj, "You drop %s!", doname(obj),
*/
        obj = hold_another_object(obj, "あなたは%sを落した！", doname(obj),
                                  (const char *) 0);
        if (obj)
            obj->nomerge = 0;
    }
}

static NEARDATA const char cuddly[] = { TOOL_CLASS, GEM_CLASS, 0 };

int
dorub()
{
    struct obj *obj = getobj(cuddly, "rub");

    if (obj && obj->oclass == GEM_CLASS) {
        if (is_graystone(obj)) {
            use_stone(obj);
            return 1;
        } else {
/*JP
            pline("Sorry, I don't know how to use that.");
*/
            pline("残念．使い方がわからない．");
            return 0;
        }
    }

    if (!obj || !wield_tool(obj, "rub"))
        return 0;

    /* now uwep is obj */
    if (uwep->otyp == MAGIC_LAMP) {
        if (uwep->spe > 0 && !rn2(3)) {
            check_unpaid_usage(uwep, TRUE); /* unusual item use */
            /* bones preparation:  perform the lamp transformation
               before releasing the djinni in case the latter turns out
               to be fatal (a hostile djinni has no chance to attack yet,
               but an indebted one who grants a wish might bestow an
               artifact which blasts the hero with lethal results) */
            uwep->otyp = OIL_LAMP;
            uwep->spe = 0; /* for safety */
            uwep->age = rn1(500, 1000);
            if (uwep->lamplit)
                begin_burn(uwep, TRUE);
            djinni_from_bottle(uwep);
            makeknown(MAGIC_LAMP);
            update_inventory();
        } else if (rn2(2)) {
/*JP
            You("%s smoke.", !Blind ? "see a puff of" : "smell");
*/
            pline("けむり%s．", !Blind ? "が舞いあがった" : "の匂いがした");
        } else
            pline1(nothing_happens);
    } else if (obj->otyp == BRASS_LANTERN) {
        /* message from Adventure */
/*JP
        pline("Rubbing the electric lamp is not particularly rewarding.");
*/
        pline("電気ランプをこすっても意味はないと思うが．．．");
/*JP
        pline("Anyway, nothing exciting happens.");
*/
        pline("やっぱり，何も起きなかった．");
    } else
        pline1(nothing_happens);
    return 1;
}

int
dojump()
{
    /* Physical jump */
    return jump(0);
}

boolean
is_valid_jump_pos(x, y, magic, showmsg)
int x, y, magic;
boolean showmsg;
{
    if (!magic && !(HJumping & ~INTRINSIC) && !EJumping && distu(x, y) != 5) {
        /* The Knight jumping restriction still applies when riding a
         * horse.  After all, what shape is the knight piece in chess?
         */
        if (showmsg)
/*JP
            pline("Illegal move!");
*/
            pline("その移動は桂馬跳びじゃない！");
        return FALSE;
    } else if (distu(x, y) > (magic ? 6 + magic * 3 : 9)) {
        if (showmsg)
/*JP
            pline("Too far!");
*/
            pline("遠すぎる！");
        return FALSE;
    } else if (!cansee(x, y)) {
        if (showmsg)
/*JP
            You("cannot see where to land!");
*/
            You("着地点が見えない！");
        return FALSE;
    } else if (!isok(x, y)) {
        if (showmsg)
/*JP
            You("cannot jump there!");
*/
            You("そこには飛べない！");
        return FALSE;
    }
    return TRUE;
}

static int jumping_is_magic;

void
display_jump_positions(state)
int state;
{
    if (state == 0) {
        tmp_at(DISP_BEAM, cmap_to_glyph(S_goodpos));
    } else if (state == 1) {
        int x, y, dx, dy;

        for (dx = -4; dx <= 4; dx++)
            for (dy = -4; dy <= 4; dy++) {
                x = dx + (int) u.ux;
                y = dy + (int) u.uy;
                if (isok(x, y) && ACCESSIBLE(levl[x][y].typ)
                    && is_valid_jump_pos(x, y, jumping_is_magic, FALSE))
                    tmp_at(x, y);
            }
    } else {
        tmp_at(DISP_END, 0);
    }
}

int
jump(magic)
int magic; /* 0=Physical, otherwise skill level */
{
    coord cc;

    if (!magic && (nolimbs(youmonst.data) || slithy(youmonst.data))) {
        /* normally (nolimbs || slithy) implies !Jumping,
           but that isn't necessarily the case for knights */
/*JP
        You_cant("jump; you have no legs!");
*/
        pline("足が無くては跳べない！");
        return 0;
    } else if (!magic && !Jumping) {
/*JP
        You_cant("jump very far.");
*/
        You_cant("そんな遠くまで跳べない．");
        return 0;
    /* if steed is immobile, can't do physical jump but can do spell one */
    } else if (!magic && u.usteed && stucksteed(FALSE)) {
        /* stucksteed gave "<steed> won't move" message */
        return 0;
    } else if (u.uswallow) {
        if (magic) {
/*JP
            You("bounce around a little.");
*/
            pline("反動をつけた．");
            return 1;
        }
/*JP
        pline("You've got to be kidding!");
*/
        pline("冗談はよしこさん！");
        return 0;
    } else if (u.uinwater) {
        if (magic) {
/*JP
            You("swish around a little.");
*/
            pline("スイスイと泳いだ．");
            return 1;
        }
/*JP
        pline("This calls for swimming, not jumping!");
*/
        pline("それは『泳ぐ』であって，『跳ぶ』じゃない！");
        return 0;
    } else if (u.ustuck) {
        if (u.ustuck->mtame && !Conflict && !u.ustuck->mconf) {
/*JP
            You("pull free from %s.", mon_nam(u.ustuck));
*/
            You("%sから離れた．", mon_nam(u.ustuck));
            u.ustuck = 0;
            return 1;
        }
        if (magic) {
/*JP
            You("writhe a little in the grasp of %s!", mon_nam(u.ustuck));
*/
            You("%sから逃れようとジタバタした！", mon_nam(u.ustuck));
            return 1;
        }
/*JP
        You("cannot escape from %s!", mon_nam(u.ustuck));
*/
        You("%sから逃れられない！", mon_nam(u.ustuck));
        return 0;
    } else if (Levitation || Is_airlevel(&u.uz) || Is_waterlevel(&u.uz)) {
        if (magic) {
/*JP
            You("flail around a little.");
*/
            You("バタバタ跳んだ．");
            return 1;
        }
/*JP
        You("don't have enough traction to jump.");
*/
        You("跳ぶための反動がつけられない．");
        return 0;
    } else if (!magic && near_capacity() > UNENCUMBERED) {
/*JP
        You("are carrying too much to jump!");
*/
        You("たくさん物を持ちすぎて跳べない！");
        return 0;
    } else if (!magic && (u.uhunger <= 100 || ACURR(A_STR) < 6)) {
/*JP
        You("lack the strength to jump!");
*/
        You("跳ぶだけの力がない！");
        return 0;
    } else if (!magic && Wounded_legs) {
        long wl = (Wounded_legs & BOTH_SIDES);
        const char *bp = body_part(LEG);

        if (wl == BOTH_SIDES)
            bp = makeplural(bp);
        if (u.usteed)
/*JP
            pline("%s is in no shape for jumping.", Monnam(u.usteed));
*/
            pline("%sは跳べる状態ではない．", Monnam(u.usteed));
        else
#if 0 /*JP*/
            Your("%s%s %s in no shape for jumping.",
                 (wl == LEFT_SIDE) ? "left " : (wl == RIGHT_SIDE) ? "right "
                                                                  : "",
                 bp, (wl == BOTH_SIDES) ? "are" : "is");
#else
          Your("%s%sは跳べる状態ではない．",
                 (wl == LEFT_SIDE) ? "左" :
                 (wl == RIGHT_SIDE) ? "右" : "", bp);
#endif
        return 0;
    } else if (u.usteed && u.utrap) {
/*JP
        pline("%s is stuck in a trap.", Monnam(u.usteed));
*/
        pline("%sは罠にひっかかっている．", Monnam(u.usteed));
        return 0;
    }

/*JP
    pline("Where do you want to jump?");
*/
    pline("どこに跳びますか？");
    cc.x = u.ux;
    cc.y = u.uy;
    jumping_is_magic = magic;
    getpos_sethilite(display_jump_positions);
/*JP
    if (getpos(&cc, TRUE, "the desired position") < 0)
*/
    if (getpos(&cc, TRUE, "跳びたい場所") < 0)
        return 0; /* user pressed ESC */
    if (!is_valid_jump_pos(cc.x, cc.y, magic, TRUE)) {
        return 0;
    } else {
        coord uc;
        int range, temp;

        if (u.utrap)
            switch (u.utraptype) {
            case TT_BEARTRAP: {
                long side = rn2(3) ? LEFT_SIDE : RIGHT_SIDE;

/*JP
                You("rip yourself free of the bear trap!  Ouch!");
*/
                You("自分を熊の罠からひきはがした，いてっ！");
/*JP
                losehp(Maybe_Half_Phys(rnd(10)), "jumping out of a bear trap",
*/
                losehp(Maybe_Half_Phys(rnd(10)), "熊の罠から飛び出ようとして",
                       KILLED_BY);
                set_wounded_legs(side, rn1(1000, 500));
                break;
            }
            case TT_PIT:
/*JP
                You("leap from the pit!");
*/
                You("落し穴から飛び出た！");
                break;
            case TT_WEB:
/*JP
                You("tear the web apart as you pull yourself free!");
*/
                You("くもの巣を引き裂き，自由になった！");
                deltrap(t_at(u.ux, u.uy));
                break;
            case TT_LAVA:
/*JP
                You("pull yourself above the lava!");
*/
                You("溶岩から飛び出た！");
                u.utrap = 0;
                return 1;
            case TT_BURIEDBALL:
            case TT_INFLOOR:
#if 0 /*JP*/
                You("strain your %s, but you're still %s.",
                    makeplural(body_part(LEG)),
                    (u.utraptype == TT_INFLOOR)
                        ? "stuck in the floor"
                        : "attached to the buried ball");
#else
                You("%sを引っぱったが，あなたはまだ%s．",
                    makeplural(body_part(LEG)),
                    (u.utraptype == TT_INFLOOR)
                        ? "床にうまっている"
                        : "うまった球とつながっている");
#endif
                set_wounded_legs(LEFT_SIDE, rn1(10, 11));
                set_wounded_legs(RIGHT_SIDE, rn1(10, 11));
                return 1;
            }

        /*
         * Check the path from uc to cc, calling hurtle_step at each
         * location.  The final position actually reached will be
         * in cc.
         */
        uc.x = u.ux;
        uc.y = u.uy;
        /* calculate max(abs(dx), abs(dy)) as the range */
        range = cc.x - uc.x;
        if (range < 0)
            range = -range;
        temp = cc.y - uc.y;
        if (temp < 0)
            temp = -temp;
        if (range < temp)
            range = temp;
        (void) walk_path(&uc, &cc, hurtle_step, (genericptr_t) &range);
        teleds(cc.x, cc.y, TRUE);
        sokoban_guilt();
        nomul(-1);
        multi_reason = "jumping around";
        nomovemsg = "";
        morehungry(rnd(25));
        return 1;
    }
}

boolean
tinnable(corpse)
struct obj *corpse;
{
    if (corpse->oeaten)
        return 0;
    if (!mons[corpse->corpsenm].cnutrit)
        return 0;
    return 1;
}

STATIC_OVL void
use_tinning_kit(obj)
struct obj *obj;
{
    struct obj *corpse, *can;

    /* This takes only 1 move.  If this is to be changed to take many
     * moves, we've got to deal with decaying corpses...
     */
    if (obj->spe <= 0) {
/*JP
        You("seem to be out of tins.");
*/
        pline("缶詰を作るための缶が切れたようだ．");
        return;
    }
    if (!(corpse = floorfood("tin", 2)))
        return;
    if (corpse->oeaten) {
/*JP
        You("cannot tin %s which is partly eaten.", something);
*/
        You("食べかけのものを缶詰にすることはできない．");
        return;
    }
    if (touch_petrifies(&mons[corpse->corpsenm]) && !Stone_resistance
        && !uarmg) {
        char kbuf[BUFSZ];

        if (poly_when_stoned(youmonst.data))
#if 0 /*JP*/
            You("tin %s without wearing gloves.",
                an(mons[corpse->corpsenm].mname));
#else
            You("小手なしで%sを缶詰にしようとした．",
                mons[corpse->corpsenm].mname);
#endif
        else {
#if 0 /*JP*/
            pline("Tinning %s without wearing gloves is a fatal mistake...",
                  an(mons[corpse->corpsenm].mname));
#else
            pline("%sを小手なしで缶詰にするのは致命的な間違いだ．．．",
                  mons[corpse->corpsenm].mname);
#endif
#if 0 /*JP*/
            Sprintf(kbuf, "trying to tin %s without gloves",
                    an(mons[corpse->corpsenm].mname));
#else
            Sprintf(kbuf, "小手をつけずに%sを缶詰にしようとして",
                    mons[corpse->corpsenm].mname);
#endif
        }
        instapetrify(kbuf);
    }
    if (is_rider(&mons[corpse->corpsenm])) {
        if (revive_corpse(corpse))
/*JP
            verbalize("Yes...  But War does not preserve its enemies...");
*/
            verbalize("そうだ．．．しかし「戦争」は敵に安らぎを与えぬ．．．");
        else
/*JP
            pline_The("corpse evades your grasp.");
*/
            pline("死体はあなたの手を逃れた．");
        return;
    }
    if (mons[corpse->corpsenm].cnutrit == 0) {
/*JP
        pline("That's too insubstantial to tin.");
*/
        pline("実体がないので缶詰にできない．");
        return;
    }
    consume_obj_charge(obj, TRUE);

    if ((can = mksobj(TIN, FALSE, FALSE)) != 0) {
/*JP
        static const char you_buy_it[] = "You tin it, you bought it!";
*/
        static const char you_buy_it[] = "缶詰にしたのなら買ってもらうよ！";

        can->corpsenm = corpse->corpsenm;
        can->cursed = obj->cursed;
        can->blessed = obj->blessed;
        can->owt = weight(can);
        can->known = 1;
        /* Mark tinned tins. No spinach allowed... */
        set_tin_variety(can, HOMEMADE_TIN);
        if (carried(corpse)) {
            if (corpse->unpaid)
                verbalize(you_buy_it);
            useup(corpse);
        } else {
            if (costly_spot(corpse->ox, corpse->oy) && !corpse->no_charge)
                verbalize(you_buy_it);
            useupf(corpse, 1L);
        }
/*JP
        can = hold_another_object(can, "You make, but cannot pick up, %s.",
*/
        can = hold_another_object(can, "缶詰にできたが，%sを持つことができない．",
                                  doname(can), (const char *) 0);
    } else
        impossible("Tinning failed.");
}

void
use_unicorn_horn(obj)
struct obj *obj;
{
#define PROP_COUNT 7           /* number of properties we're dealing with */
#define ATTR_COUNT (A_MAX * 3) /* number of attribute points we might fix */
    int idx, val, val_limit, trouble_count, unfixable_trbl, did_prop,
        did_attr;
    int trouble_list[PROP_COUNT + ATTR_COUNT];

    if (obj && obj->cursed) {
        long lcount = (long) rn1(90, 10);

        switch (rn2(13) / 2) { /* case 6 is half as likely as the others */
        case 0:
            make_sick((Sick & TIMEOUT) ? (Sick & TIMEOUT) / 3L + 1L
                                       : (long) rn1(ACURR(A_CON), 20),
                      xname(obj), TRUE, SICK_NONVOMITABLE);
            break;
        case 1:
            make_blinded((Blinded & TIMEOUT) + lcount, TRUE);
            break;
        case 2:
            if (!Confusion)
#if 0 /*JP*/
                You("suddenly feel %s.",
                    Hallucination ? "trippy" : "confused");
#else
                You("突然%s．",
                    Hallucination ? "へろへろになった" : "混乱した");
#endif
            make_confused((HConfusion & TIMEOUT) + lcount, TRUE);
            break;
        case 3:
            make_stunned((HStun & TIMEOUT) + lcount, TRUE);
            break;
        case 4:
            (void) adjattrib(rn2(A_MAX), -1, FALSE);
            break;
        case 5:
            (void) make_hallucinated((HHallucination & TIMEOUT) + lcount,
                                     TRUE, 0L);
            break;
        case 6:
            if (Deaf) /* make_deaf() won't give feedback when already deaf */
/*JP
                pline("Nothing seems to happen.");
*/
                pline("何も起きなかったようだ．");
            make_deaf((HDeaf & TIMEOUT) + lcount, TRUE);
            break;
        }
        return;
    }

/*
 * Entries in the trouble list use a very simple encoding scheme.
 */
#define prop2trbl(X) ((X) + A_MAX)
#define attr2trbl(Y) (Y)
#define prop_trouble(X) trouble_list[trouble_count++] = prop2trbl(X)
#define attr_trouble(Y) trouble_list[trouble_count++] = attr2trbl(Y)
#define TimedTrouble(P) (((P) && !((P) & ~TIMEOUT)) ? ((P) & TIMEOUT) : 0L)

    trouble_count = unfixable_trbl = did_prop = did_attr = 0;

    /* collect property troubles */
    if (TimedTrouble(Sick))
        prop_trouble(SICK);
    if (TimedTrouble(Blinded) > (long) u.ucreamed
        && !(u.uswallow
             && attacktype_fordmg(u.ustuck->data, AT_ENGL, AD_BLND)))
        prop_trouble(BLINDED);
    if (TimedTrouble(HHallucination))
        prop_trouble(HALLUC);
    if (TimedTrouble(Vomiting))
        prop_trouble(VOMITING);
    if (TimedTrouble(HConfusion))
        prop_trouble(CONFUSION);
    if (TimedTrouble(HStun))
        prop_trouble(STUNNED);
    if (TimedTrouble(HDeaf))
        prop_trouble(DEAF);

    unfixable_trbl = unfixable_trouble_count(TRUE);

    /* collect attribute troubles */
    for (idx = 0; idx < A_MAX; idx++) {
        if (ABASE(idx) >= AMAX(idx))
            continue;
        val_limit = AMAX(idx);
        /* don't recover strength lost from hunger */
        if (idx == A_STR && u.uhs >= WEAK)
            val_limit--;
        if (Fixed_abil) {
            /* potion/spell of restore ability override sustain ability
               intrinsic but unicorn horn usage doesn't */
            unfixable_trbl += val_limit - ABASE(idx);
            continue;
        }
        /* don't recover more than 3 points worth of any attribute */
        if (val_limit > ABASE(idx) + 3)
            val_limit = ABASE(idx) + 3;

        for (val = ABASE(idx); val < val_limit; val++)
            attr_trouble(idx);
        /* keep track of unfixed trouble, for message adjustment below */
        unfixable_trbl += (AMAX(idx) - val_limit);
    }

    if (trouble_count == 0) {
        pline1(nothing_happens);
        return;
    } else if (trouble_count > 1) { /* shuffle */
        int i, j, k;

        for (i = trouble_count - 1; i > 0; i--)
            if ((j = rn2(i + 1)) != i) {
                k = trouble_list[j];
                trouble_list[j] = trouble_list[i];
                trouble_list[i] = k;
            }
    }

    /*
     *  Chances for number of troubles to be fixed
     *               0      1      2      3      4      5      6      7
     *   blessed:  22.7%  22.7%  19.5%  15.4%  10.7%   5.7%   2.6%   0.8%
     *  uncursed:  35.4%  35.4%  22.9%   6.3%    0      0      0      0
     */
    val_limit = rn2(d(2, (obj && obj->blessed) ? 4 : 2));
    if (val_limit > trouble_count)
        val_limit = trouble_count;

    /* fix [some of] the troubles */
    for (val = 0; val < val_limit; val++) {
        idx = trouble_list[val];

        switch (idx) {
        case prop2trbl(SICK):
            make_sick(0L, (char *) 0, TRUE, SICK_ALL);
            did_prop++;
            break;
        case prop2trbl(BLINDED):
            make_blinded((long) u.ucreamed, TRUE);
            did_prop++;
            break;
        case prop2trbl(HALLUC):
            (void) make_hallucinated(0L, TRUE, 0L);
            did_prop++;
            break;
        case prop2trbl(VOMITING):
            make_vomiting(0L, TRUE);
            did_prop++;
            break;
        case prop2trbl(CONFUSION):
            make_confused(0L, TRUE);
            did_prop++;
            break;
        case prop2trbl(STUNNED):
            make_stunned(0L, TRUE);
            did_prop++;
            break;
        case prop2trbl(DEAF):
            make_deaf(0L, TRUE);
            did_prop++;
            break;
        default:
            if (idx >= 0 && idx < A_MAX) {
                ABASE(idx) += 1;
                did_attr++;
            } else
                panic("use_unicorn_horn: bad trouble? (%d)", idx);
            break;
        }
    }

    if (did_attr)
#if 0 /*JP*/
        pline("This makes you feel %s!",
              (did_prop + did_attr) == (trouble_count + unfixable_trbl)
                  ? "great"
                  : "better");
#else
        pline("気分が%sよくなった！",
              (did_prop + did_attr) == (trouble_count + unfixable_trbl)
                  ? "とても"
                  : "より");
#endif
    else if (!did_prop)
/*JP
        pline("Nothing seems to happen.");
*/
        pline("何も起きなかったようだ．");

    context.botl = (did_attr || did_prop);
#undef PROP_COUNT
#undef ATTR_COUNT
#undef prop2trbl
#undef attr2trbl
#undef prop_trouble
#undef attr_trouble
#undef TimedTrouble
}

/*
 * Timer callback routine: turn figurine into monster
 */
void
fig_transform(arg, timeout)
anything *arg;
long timeout;
{
    struct obj *figurine = arg->a_obj;
    struct monst *mtmp;
    coord cc;
    boolean cansee_spot, silent, okay_spot;
    boolean redraw = FALSE;
    boolean suppress_see = FALSE;
    char monnambuf[BUFSZ], carriedby[BUFSZ];

    if (!figurine) {
        debugpline0("null figurine in fig_transform()");
        return;
    }
    silent = (timeout != monstermoves); /* happened while away */
    okay_spot = get_obj_location(figurine, &cc.x, &cc.y, 0);
    if (figurine->where == OBJ_INVENT || figurine->where == OBJ_MINVENT)
        okay_spot = enexto(&cc, cc.x, cc.y, &mons[figurine->corpsenm]);
    if (!okay_spot || !figurine_location_checks(figurine, &cc, TRUE)) {
        /* reset the timer to try again later */
        (void) start_timer((long) rnd(5000), TIMER_OBJECT, FIG_TRANSFORM,
                           obj_to_any(figurine));
        return;
    }

    cansee_spot = cansee(cc.x, cc.y);
    mtmp = make_familiar(figurine, cc.x, cc.y, TRUE);
    if (mtmp) {
        char and_vanish[BUFSZ];
        struct obj *mshelter = level.objects[mtmp->mx][mtmp->my];
        Sprintf(monnambuf, "%s", an(m_monnam(mtmp)));

        /*JP:TODO:and_vanishは未処理*/
        and_vanish[0] = '\0';
        if ((mtmp->minvis && !See_invisible)
            || (mtmp->data->mlet == S_MIMIC
                && mtmp->m_ap_type != M_AP_NOTHING))
            suppress_see = TRUE;

        if (mtmp->mundetected) {
            if (hides_under(mtmp->data) && mshelter) {
                Sprintf(and_vanish, " and %s under %s",
                        locomotion(mtmp->data, "crawl"), doname(mshelter));
            } else if (mtmp->data->mlet == S_MIMIC
                       || mtmp->data->mlet == S_EEL) {
                suppress_see = TRUE;
            } else
                Strcpy(and_vanish, " and vanish");
        }

        switch (figurine->where) {
        case OBJ_INVENT:
            if (Blind || suppress_see)
#if 0 /*JP*/
                You_feel("%s %s from your pack!", something,
                         locomotion(mtmp->data, "drop"));
#else
                You_feel("%sがあなたの鞄から%sようだ！", something,
                         jpast(locomotion(mtmp->data, "落ちる")));
#endif
            else
#if 0 /*JP*/
                You_see("%s %s out of your pack%s!", monnambuf,
                        locomotion(mtmp->data, "drop"), and_vanish);
#else
                You("%sがあなたの鞄から%sのを見た！", monnambuf,
                        jpast(locomotion(mtmp->data,"落ちる")));
#endif
            break;

        case OBJ_FLOOR:
            if (cansee_spot && !silent) {
                if (suppress_see)
/*JP
                    pline("%s suddenly vanishes!", an(xname(figurine)));
*/
                    pline("%sは突然消えた！", xname(figurine));
                else
#if 0 /*JP*/
                    You_see("a figurine transform into %s%s!", monnambuf,
                            and_vanish);
#else
                    You("人形が突然%sになったのを見た！",
                            monnambuf);
#endif
                redraw = TRUE; /* update figurine's map location */
            }
            break;

        case OBJ_MINVENT:
            if (cansee_spot && !silent && !suppress_see) {
                struct monst *mon;
                mon = figurine->ocarry;
                /* figurine carrying monster might be invisible */
                if (canseemon(figurine->ocarry)) {
/*JP
                    Sprintf(carriedby, "%s pack", s_suffix(a_monnam(mon)));
*/
                    Sprintf(carriedby, "%sの鞄", s_suffix(a_monnam(mon)));
                } else if (is_pool(mon->mx, mon->my))
/*JP
                    Strcpy(carriedby, "empty water");
*/
                    Strcpy(carriedby, "何もない水中");
                else
/*JP
                    Strcpy(carriedby, "thin air");
*/
                    Strcpy(carriedby, "何もない空中");
#if 0 /*JP*/
                You_see("%s %s out of %s%s!", monnambuf,
                        locomotion(mtmp->data, "drop"), carriedby,
                        and_vanish);
#else
                You("%sが%sから%sのを見た！", monnambuf,
                        carriedby, locomotion(mtmp->data, "落ちる"));
#endif
            }
            break;
#if 0
        case OBJ_MIGRATING:
            break;
#endif

        default:
            impossible("figurine came to life where? (%d)",
                       (int) figurine->where);
            break;
        }
    }
    /* free figurine now */
    if (carried(figurine)) {
        useup(figurine);
    } else {
        obj_extract_self(figurine);
        obfree(figurine, (struct obj *) 0);
    }
    if (redraw)
        newsym(cc.x, cc.y);
}

STATIC_OVL boolean
figurine_location_checks(obj, cc, quietly)
struct obj *obj;
coord *cc;
boolean quietly;
{
    xchar x, y;

    if (carried(obj) && u.uswallow) {
        if (!quietly)
/*JP
            You("don't have enough room in here.");
*/
            pline("ここには十分な場所がない．");
        return FALSE;
    }
    x = cc ? cc->x : u.ux;
    y = cc ? cc->y : u.uy;
    if (!isok(x, y)) {
        if (!quietly)
/*JP
            You("cannot put the figurine there.");
*/
            You("ここには人形を置けない．");
        return FALSE;
    }
    if (IS_ROCK(levl[x][y].typ)
        && !(passes_walls(&mons[obj->corpsenm]) && may_passwall(x, y))) {
        if (!quietly)
#if 0 /*JP*/
            You("cannot place a figurine in %s!",
                IS_TREE(levl[x][y].typ) ? "a tree" : "solid rock");
#else
            You("%sの中には人形を置けない！",
                IS_TREE(levl[x][y].typ) ? "木" : "固い石");
#endif
        return FALSE;
    }
    if (sobj_at(BOULDER, x, y) && !passes_walls(&mons[obj->corpsenm])
        && !throws_rocks(&mons[obj->corpsenm])) {
        if (!quietly)
/*JP
            You("cannot fit the figurine on the boulder.");
*/
            You("岩に人形を押し込むことはできない．");
        return FALSE;
    }
    return TRUE;
}

STATIC_OVL void
use_figurine(optr)
struct obj **optr;
{
    register struct obj *obj = *optr;
    xchar x, y;
    coord cc;

    if (u.uswallow) {
        /* can't activate a figurine while swallowed */
        if (!figurine_location_checks(obj, (coord *) 0, FALSE))
            return;
    }
    if (!getdir((char *) 0)) {
        context.move = multi = 0;
        return;
    }
    x = u.ux + u.dx;
    y = u.uy + u.dy;
    cc.x = x;
    cc.y = y;
    /* Passing FALSE arg here will result in messages displayed */
    if (!figurine_location_checks(obj, &cc, FALSE))
        return;
#if 0 /*JP*/
    You("%s and it transforms.",
        (u.dx || u.dy) ? "set the figurine beside you"
                       : (Is_airlevel(&u.uz) || Is_waterlevel(&u.uz)
                          || is_pool(cc.x, cc.y))
                             ? "release the figurine"
                             : (u.dz < 0 ? "toss the figurine into the air"
                                         : "set the figurine on the ground"));
#else
    You("%s．するとそれは変形した．",
        (u.dx || u.dy) ? "そばに人形を置いた"
                       : (Is_airlevel(&u.uz) || Is_waterlevel(&u.uz)
                          || is_pool(cc.x, cc.y))
                             ? "人形を放った"
                             : (u.dz < 0 ?  "人形を空中に投げた"
                             : "人形を地面に置いた"));
#endif
    (void) make_familiar(obj, cc.x, cc.y, FALSE);
    (void) stop_timer(FIG_TRANSFORM, obj_to_any(obj));
    useup(obj);
    *optr = 0;
}

static NEARDATA const char lubricables[] = { ALL_CLASSES, ALLOW_NONE, 0 };

STATIC_OVL void
use_grease(obj)
struct obj *obj;
{
    struct obj *otmp;

    if (Glib) {
#if 0 /*JP*/
        pline("%s from your %s.", Tobjnam(obj, "slip"),
              makeplural(body_part(FINGER)));
#else
        pline("%sはあなたの%sから滑り落ちた．", xname(obj),
              body_part(FINGER));
#endif
        dropx(obj);
        return;
    }

    if (obj->spe > 0) {
        if ((obj->cursed || Fumbling) && !rn2(2)) {
            consume_obj_charge(obj, TRUE);

#if 0 /*JP*/
            pline("%s from your %s.", Tobjnam(obj, "slip"),
                  makeplural(body_part(FINGER)));
#else
            pline("%sはあなたの%sから滑り落ちた．", xname(obj),
                  body_part(FINGER));
#endif
            dropx(obj);
            return;
        }
        otmp = getobj(lubricables, "grease");
        if (!otmp)
            return;
/*JP
        if (inaccessible_equipment(otmp, "grease", FALSE))
*/
        if (inaccessible_equipment(otmp, "に脂を塗る", FALSE))
            return;
        consume_obj_charge(obj, TRUE);

        if (otmp != &zeroobj) {
/*JP
            You("cover %s with a thick layer of grease.", yname(otmp));
*/
            You("%sに脂を丹念に塗った．", xname(otmp));
            otmp->greased = 1;
            if (obj->cursed && !nohands(youmonst.data)) {
                incr_itimeout(&Glib, rnd(15));
/*JP
                pline("Some of the grease gets all over your %s.",
*/
                pline("脂が少し%sについた．",
                      makeplural(body_part(HAND)));
            }
        } else {
            incr_itimeout(&Glib, rnd(15));
/*JP
            You("coat your %s with grease.", makeplural(body_part(FINGER)));
*/
            You("%sに脂を塗った．", makeplural(body_part(FINGER)));
        }
    } else {
        if (obj->known)
/*JP
            pline("%s empty.", Tobjnam(obj, "are"));
*/
            pline("%sは空っぽだ．", xname(obj));
        else
/*JP
            pline("%s to be empty.", Tobjnam(obj, "seem"));
*/
            pline("%sは空っぽのようだ．", xname(obj));
    }
    update_inventory();
}

static struct trapinfo {
    struct obj *tobj;
    xchar tx, ty;
    int time_needed;
    boolean force_bungle;
} trapinfo;

void
reset_trapset()
{
    trapinfo.tobj = 0;
    trapinfo.force_bungle = 0;
}

/* touchstones - by Ken Arnold */
STATIC_OVL void
use_stone(tstone)
struct obj *tstone;
{
    struct obj *obj;
    boolean do_scratch;
    const char *streak_color, *choices;
    char stonebuf[QBUFSZ];
/*JP
    static const char scritch[] = "\"scritch, scritch\"";
*/
    static const char scritch[] = "「ゴシ，ゴシ」";
    static const char allowall[3] = { COIN_CLASS, ALL_CLASSES, 0 };
    static const char coins_gems[3] = { COIN_CLASS, GEM_CLASS, 0 };

    /* in case it was acquired while blinded */
    if (!Blind)
        tstone->dknown = 1;
    /* when the touchstone is fully known, don't bother listing extra
       junk as likely candidates for rubbing */
    choices = (tstone->otyp == TOUCHSTONE && tstone->dknown
               && objects[TOUCHSTONE].oc_name_known)
                  ? coins_gems
                  : allowall;
/*JP
    Sprintf(stonebuf, "rub on the stone%s", plur(tstone->quan));
*/
    Sprintf(stonebuf, "rub on the stone");
    if ((obj = getobj(choices, stonebuf)) == 0)
        return;

    if (obj == tstone && obj->quan == 1L) {
/*JP
        You_cant("rub %s on itself.", the(xname(obj)));
*/
        You("%sをそれ自体でこすることはできない．", the(xname(obj)));
        return;
    }

    if (tstone->otyp == TOUCHSTONE && tstone->cursed
        && obj->oclass == GEM_CLASS && !is_graystone(obj)
        && !obj_resists(obj, 80, 100)) {
        if (Blind)
/*JP
            pline("You feel something shatter.");
*/
            You("何かが粉々になるのを感じた．");
        else if (Hallucination)
/*JP
            pline("Oh, wow, look at the pretty shards.");
*/
            pline("ワーォ！なんてきれいな破片なんだ．");
        else
#if 0 /*JP*/
            pline("A sharp crack shatters %s%s.",
                  (obj->quan > 1L) ? "one of " : "", the(xname(obj)));
#else
            pline("強くこすりすぎて%s%sは粉々になってしまった．",
                  the(xname(obj)), (obj->quan > 1) ? "のひとつ" : "");
#endif
        useup(obj);
        return;
    }

    if (Blind) {
        pline(scritch);
        return;
    } else if (Hallucination) {
#if 0 /*JP*/
        pline("Oh wow, man: Fractals!");
#else
        pline("ワーォ！フラクタル模様だ！");
#endif
        return;
    }

    do_scratch = FALSE;
    streak_color = 0;

    switch (obj->oclass) {
    case GEM_CLASS: /* these have class-specific handling below */
    case RING_CLASS:
        if (tstone->otyp != TOUCHSTONE) {
            do_scratch = TRUE;
        } else if (obj->oclass == GEM_CLASS
                   && (tstone->blessed
                       || (!tstone->cursed && (Role_if(PM_ARCHEOLOGIST)
                                               || Race_if(PM_GNOME))))) {
            makeknown(TOUCHSTONE);
            makeknown(obj->otyp);
            prinv((char *) 0, obj, 0L);
            return;
        } else {
            /* either a ring or the touchstone was not effective */
            if (objects[obj->otyp].oc_material == GLASS) {
                do_scratch = TRUE;
                break;
            }
        }
        streak_color = c_obj_colors[objects[obj->otyp].oc_color];
        break; /* gem or ring */

    default:
        switch (objects[obj->otyp].oc_material) {
        case CLOTH:
#if 0 /*JP*/
            pline("%s a little more polished now.", Tobjnam(tstone, "look"));
#else
            pline("%sはさらにつやが出たように見える．", xname(tstone));
#endif
            return;
        case LIQUID:
            if (!obj->known) /* note: not "whetstone" */
#if 0 /*JP*/
                You("must think this is a wetstone, do you?");
#else
                You("これは砥石だと思った？");
#endif
            else
#if 0 /*JP*/
                pline("%s a little wetter now.", Tobjnam(tstone, "are"));
#else
                pline("%sは少しぬれた．", xname(tstone));
#endif
            return;
        case WAX:
#if 0 /*JP*/
            streak_color = "waxy";
#else
            streak_color = "脂っぽい";
#endif
            break; /* okay even if not touchstone */
        case WOOD:
#if 0 /*JP*/
            streak_color = "wooden";
#else
            streak_color = "おがくずのような";
#endif
            break; /* okay even if not touchstone */
        case GOLD:
            do_scratch = TRUE; /* scratching and streaks */
#if 0 /*JP*/
            streak_color = "golden";
#else
            streak_color = "金色の";
#endif
            break;
        case SILVER:
            do_scratch = TRUE; /* scratching and streaks */
#if 0 /*JP*/
            streak_color = "silvery";
#else
            streak_color = "銀色の";
#endif
            break;
        default:
            /* Objects passing the is_flimsy() test will not
               scratch a stone.  They will leave streaks on
               non-touchstones and touchstones alike. */
            if (is_flimsy(obj))
                streak_color = c_obj_colors[objects[obj->otyp].oc_color];
            else
                do_scratch = (tstone->otyp != TOUCHSTONE);
            break;
        }
        break; /* default oclass */
    }

#if 0 /*JP*//* not used */
    Sprintf(stonebuf, "stone%s", plur(tstone->quan));
#endif
    if (do_scratch)
#if 0 /*JP*/
        You("make %s%sscratch marks on the %s.",
            streak_color ? streak_color : (const char *) "",
            streak_color ? " " : "", stonebuf);
#else
        You("%sこすり跡を石につけた．",
            streak_color ? streak_color : (const char *)"");
#endif
    else if (streak_color)
/*JP
        You_see("%s streaks on the %s.", streak_color, stonebuf);
*/
        pline("石に%s筋がついた．", streak_color);
    else
        pline(scritch);
    return;
}

/* Place a landmine/bear trap.  Helge Hafting */
STATIC_OVL void
use_trap(otmp)
struct obj *otmp;
{
    int ttyp, tmp;
    const char *what = (char *) 0;
    char buf[BUFSZ];
    int levtyp = levl[u.ux][u.uy].typ;
#if 0 /*JP*/
    const char *occutext = "setting the trap";
#else
    const char *occutext = "罠を仕掛けている";
#endif

    if (nohands(youmonst.data))
/*JP
        what = "without hands";
*/
        what = "手がないので";
    else if (Stunned)
/*JP
        what = "while stunned";
*/
        what = "くらくらしているので";
    else if (u.uswallow)
        what =
/*JP
            is_animal(u.ustuck->data) ? "while swallowed" : "while engulfed";
*/
            is_animal(u.ustuck->data) ? "飲み込まれている間は" : "巻き込まれている間は";
    else if (Underwater)
/*JP
        what = "underwater";
*/
        what = "水面下では";
    else if (Levitation)
/*JP
        what = "while levitating";
*/
        what = "浮いている間は";
    else if (is_pool(u.ux, u.uy))
/*JP
        what = "in water";
*/
        what = "水中では";
    else if (is_lava(u.ux, u.uy))
/*JP
        what = "in lava";
*/
        what = "溶岩の中では";
    else if (On_stairs(u.ux, u.uy))
#if 0 /*JP*/
        what = (u.ux == xdnladder || u.ux == xupladder) ? "on the ladder"
                                                        : "on the stairs";
#else
        what = (u.ux == xdnladder || u.ux == xupladder) ? "はしごの上では"
                                                        : "階段の上では";
#endif
    else if (IS_FURNITURE(levtyp) || IS_ROCK(levtyp)
             || closed_door(u.ux, u.uy) || t_at(u.ux, u.uy))
/*JP
        what = "here";
*/
        what = "ここでは";
    else if (Is_airlevel(&u.uz) || Is_waterlevel(&u.uz))
#if 0 /*JP*/
        what = (levtyp == AIR)
                   ? "in midair"
                   : (levtyp == CLOUD)
                         ? "in a cloud"
                         : "in this place"; /* Air/Water Plane catch-all */
#else
        what = (levtyp == AIR)
                   ? "空中では"
                   : (levtyp == CLOUD)
                         ? "雲の中では"
                         : "ここでは"; /* Air/Water Plane catch-all */
#endif
    if (what) {
/*JP
        You_cant("set a trap %s!", what);
*/
        pline("%s罠を仕掛けられない！",what);
        reset_trapset();
        return;
    }
    ttyp = (otmp->otyp == LAND_MINE) ? LANDMINE : BEAR_TRAP;
    if (otmp == trapinfo.tobj && u.ux == trapinfo.tx && u.uy == trapinfo.ty) {
/*JP
        You("resume setting %s%s.", shk_your(buf, otmp),
*/
        You("%sを仕掛けるのを再開した．",
            defsyms[trap_to_defsym(what_trap(ttyp))].explanation);
        set_occupation(set_trap, occutext, 0);
        return;
    }
    trapinfo.tobj = otmp;
    trapinfo.tx = u.ux, trapinfo.ty = u.uy;
    tmp = ACURR(A_DEX);
    trapinfo.time_needed =
        (tmp > 17) ? 2 : (tmp > 12) ? 3 : (tmp > 7) ? 4 : 5;
    if (Blind)
        trapinfo.time_needed *= 2;
    tmp = ACURR(A_STR);
    if (ttyp == BEAR_TRAP && tmp < 18)
        trapinfo.time_needed += (tmp > 12) ? 1 : (tmp > 7) ? 2 : 4;
    /*[fumbling and/or confusion and/or cursed object check(s)
       should be incorporated here instead of in set_trap]*/
    if (u.usteed && P_SKILL(P_RIDING) < P_BASIC) {
        boolean chance;

        if (Fumbling || otmp->cursed)
            chance = (rnl(10) > 3);
        else
            chance = (rnl(10) > 5);
/*JP
        You("aren't very skilled at reaching from %s.", mon_nam(u.usteed));
*/
        pline("%sの上からではうまく仕掛けられないかもしれない．", mon_nam(u.usteed));
/*JP
        Sprintf(buf, "Continue your attempt to set %s?",
*/
        Sprintf(buf, "%sの仕掛けを続ける？",
                the(defsyms[trap_to_defsym(what_trap(ttyp))].explanation));
        if (yn(buf) == 'y') {
            if (chance) {
                switch (ttyp) {
                case LANDMINE: /* set it off */
                    trapinfo.time_needed = 0;
                    trapinfo.force_bungle = TRUE;
                    break;
                case BEAR_TRAP: /* drop it without arming it */
                    reset_trapset();
/*JP
                    You("drop %s!",
*/
                    You("%sを落とした！",
                        the(defsyms[trap_to_defsym(what_trap(ttyp))]
                                .explanation));
                    dropx(otmp);
                    return;
                }
            }
        } else {
            reset_trapset();
            return;
        }
    }
/*JP
    You("begin setting %s%s.", shk_your(buf, otmp),
*/
    You("%s%sを仕掛けはじめた．", shk_your(buf, otmp),
        defsyms[trap_to_defsym(what_trap(ttyp))].explanation);
    set_occupation(set_trap, occutext, 0);
    return;
}

STATIC_PTR
int
set_trap()
{
    struct obj *otmp = trapinfo.tobj;
    struct trap *ttmp;
    int ttyp;

    if (!otmp || !carried(otmp) || u.ux != trapinfo.tx
        || u.uy != trapinfo.ty) {
        /* ?? */
        reset_trapset();
        return 0;
    }

    if (--trapinfo.time_needed > 0)
        return 1; /* still busy */

    ttyp = (otmp->otyp == LAND_MINE) ? LANDMINE : BEAR_TRAP;
    ttmp = maketrap(u.ux, u.uy, ttyp);
    if (ttmp) {
        ttmp->madeby_u = 1;
        feeltrap(ttmp);
        if (*in_rooms(u.ux, u.uy, SHOPBASE)) {
            add_damage(u.ux, u.uy, 0L); /* schedule removal */
        }
        if (!trapinfo.force_bungle)
/*JP
            You("finish arming %s.",
*/
            You("%sを仕掛け終えた．",
                the(defsyms[trap_to_defsym(what_trap(ttyp))].explanation));
        if (((otmp->cursed || Fumbling) && (rnl(10) > 5))
            || trapinfo.force_bungle)
            dotrap(ttmp,
                   (unsigned) (trapinfo.force_bungle ? FORCEBUNGLE : 0));
    } else {
        /* this shouldn't happen */
/*JP
        Your("trap setting attempt fails.");
*/
        You("罠を仕掛けるのに失敗した．");
    }
    useup(otmp);
    reset_trapset();
    return 0;
}

STATIC_OVL int
use_whip(obj)
struct obj *obj;
{
    char buf[BUFSZ];
    struct monst *mtmp;
    struct obj *otmp;
    int rx, ry, proficient, res = 0;
/*JP
    const char *msg_slipsfree = "The bullwhip slips free.";
*/
    const char *msg_slipsfree = "鞭はほどけた．";
/*JP
    const char *msg_snap = "Snap!";
*/
    const char *msg_snap = "ピシッ！";

    if (obj != uwep) {
        if (!wield_tool(obj, "lash"))
            return 0;
        else
            res = 1;
    }
    if (!getdir((char *) 0))
        return res;

    if (u.uswallow) {
        mtmp = u.ustuck;
        rx = mtmp->mx;
        ry = mtmp->my;
    } else {
        if (Stunned || (Confusion && !rn2(5)))
            confdir();
        rx = u.ux + u.dx;
        ry = u.uy + u.dy;
        if (!isok(rx, ry)) {
/*JP
            You("miss.");
*/
            You("はずした．");
            return res;
        }
        mtmp = m_at(rx, ry);
    }

    /* fake some proficiency checks */
    proficient = 0;
    if (Role_if(PM_ARCHEOLOGIST))
        ++proficient;
    if (ACURR(A_DEX) < 6)
        proficient--;
    else if (ACURR(A_DEX) >= 14)
        proficient += (ACURR(A_DEX) - 14);
    if (Fumbling)
        --proficient;
    if (proficient > 3)
        proficient = 3;
    if (proficient < 0)
        proficient = 0;

    if (u.uswallow && attack(u.ustuck)) {
/*JP
        There("is not enough room to flick your bullwhip.");
*/
        pline("鞭を打つほど広くない．");

    } else if (Underwater) {
/*JP
        There("is too much resistance to flick your bullwhip.");
*/
        pline("水の抵抗がありすぎて鞭を打つことができない．");

    } else if (u.dz < 0) {
/*JP
        You("flick a bug off of the %s.", ceiling(u.ux, u.uy));
*/
        You("%sの虫を打ち落した．",ceiling(u.ux,u.uy));

    } else if ((!u.dx && !u.dy) || (u.dz > 0)) {
        int dam;

        /* Sometimes you hit your steed by mistake */
        if (u.usteed && !rn2(proficient + 2)) {
/*JP
            You("whip %s!", mon_nam(u.usteed));
*/
            You("%sを鞭打った！", mon_nam(u.usteed));
            kick_steed();
            return 1;
        }
        if (Levitation || u.usteed) {
            /* Have a shot at snaring something on the floor */
            otmp = level.objects[u.ux][u.uy];
            if (otmp && otmp->otyp == CORPSE && otmp->corpsenm == PM_HORSE) {
/*JP
                pline("Why beat a dead horse?");
*/
                pline("どうして死んだ馬を鞭打つようなことをするの？");
                return 1;
            }
            if (otmp && proficient) {
#if 0 /*JP*/
                You("wrap your bullwhip around %s on the %s.",
                    an(singular(otmp, xname)), surface(u.ux, u.uy));
#else
                You("鞭を%sの上の%sにからませた．",
                    surface(u.ux, u.uy), an(singular(otmp, xname)));
#endif
                if (rnl(6) || pickup_object(otmp, 1L, TRUE) < 1)
                    pline1(msg_slipsfree);
                return 1;
            }
        }
        dam = rnd(2) + dbon() + obj->spe;
        if (dam <= 0)
            dam = 1;
/*JP
        You("hit your %s with your bullwhip.", body_part(FOOT));
*/
        You("自分の%sを自分で打ちつけた．", body_part(FOOT));
#if 0 /*JP*/
        Sprintf(buf, "killed %sself with %s bullwhip", uhim(), uhis());
        losehp(Maybe_Half_Phys(dam), buf, NO_KILLER_PREFIX);
#else
        Strcpy(buf, "自分自身を鞭打って");
        losehp(dam, buf, KILLED_BY);
#endif
        context.botl = 1;
        return 1;

    } else if ((Fumbling || Glib) && !rn2(5)) {
/*JP
        pline_The("bullwhip slips out of your %s.", body_part(HAND));
*/
        pline("鞭が%sからすべり落ちた．", body_part(HAND));
        dropx(obj);

    } else if (u.utrap && u.utraptype == TT_PIT) {
        /*
         * Assumptions:
         *
         * if you're in a pit
         *    - you are attempting to get out of the pit
         * or, if you are applying it towards a small monster
         *    - then it is assumed that you are trying to hit it
         * else if the monster is wielding a weapon
         *    - you are attempting to disarm a monster
         * else
         *    - you are attempting to hit the monster.
         *
         * if you're confused (and thus off the mark)
         *    - you only end up hitting.
         *
         */
        const char *wrapped_what = (char *) 0;

        if (mtmp) {
            if (bigmonst(mtmp->data)) {
                wrapped_what = strcpy(buf, mon_nam(mtmp));
            } else if (proficient) {
                if (attack(mtmp))
                    return 1;
                else
                    pline1(msg_snap);
            }
        }
        if (!wrapped_what) {
            if (IS_FURNITURE(levl[rx][ry].typ))
                wrapped_what = something;
            else if (sobj_at(BOULDER, rx, ry))
/*JP
                wrapped_what = "a boulder";
*/
                wrapped_what = "岩";
        }
        if (wrapped_what) {
            coord cc;

            cc.x = rx;
            cc.y = ry;
/*JP
            You("wrap your bullwhip around %s.", wrapped_what);
*/
            You("鞭を%sにからませた．", wrapped_what);
            if (proficient && rn2(proficient + 2)) {
                if (!mtmp || enexto(&cc, rx, ry, youmonst.data)) {
/*JP
                    You("yank yourself out of the pit!");
*/
                    You("ぐいと引っぱって穴から抜け出した！");
                    teleds(cc.x, cc.y, TRUE);
                    u.utrap = 0;
                    vision_full_recalc = 1;
                }
            } else {
                pline1(msg_slipsfree);
            }
            if (mtmp)
                wakeup(mtmp);
        } else
            pline1(msg_snap);

    } else if (mtmp) {
        if (!canspotmon(mtmp) && !glyph_is_invisible(levl[rx][ry].glyph)) {
/*JP
            pline("A monster is there that you couldn't see.");
*/
            pline("見えない怪物がいる．");
            map_invisible(rx, ry);
        }
        otmp = MON_WEP(mtmp); /* can be null */
        if (otmp) {
            char onambuf[BUFSZ];
            const char *mon_hand;
            boolean gotit = proficient && (!Fumbling || !rn2(10));

            Strcpy(onambuf, cxname(otmp));
            if (gotit) {
                mon_hand = mbodypart(mtmp, HAND);
                if (bimanual(otmp))
                    mon_hand = makeplural(mon_hand);
            } else
                mon_hand = 0; /* lint suppression */

/*JP
            You("wrap your bullwhip around %s.", yname(otmp));
*/
            You("鞭を%sにからませた．", xname(otmp));
            if (gotit && mwelded(otmp)) {
#if 0 /*JP*/
                pline("%s welded to %s %s%c",
                      (otmp->quan == 1L) ? "It is" : "They are", mhis(mtmp),
                      mon_hand, !otmp->bknown ? '!' : '.');
#else
                pline("%sは%sの%sにくっついてしまっている%s",
                      onambuf,
                      mon_nam(mtmp), mon_hand,
                      !otmp->bknown ? "！" : "．");
#endif
                otmp->bknown = 1;
                gotit = FALSE; /* can't pull it free */
            }
            if (gotit) {
                obj_extract_self(otmp);
                possibly_unwield(mtmp, FALSE);
                setmnotwielded(mtmp, otmp);

                switch (rn2(proficient + 1)) {
                case 2:
                    /* to floor near you */
/*JP
                    You("yank %s to the %s!", yname(otmp),
*/
                    You("%sを%sに引き落した！", xname(otmp),
                        surface(u.ux, u.uy));
                    place_object(otmp, u.ux, u.uy);
                    stackobj(otmp);
                    break;
                case 3:
#if 0
                    /* right to you */
                    if (!rn2(25)) {
                        /* proficient with whip, but maybe not
                           so proficient at catching weapons */
                        int hitu, hitvalu;

                        hitvalu = 8 + otmp->spe;
                        hitu = thitu(hitvalu,
                                     dmgval(otmp, &youmonst),
                                     otmp, (char *)0);
                        if (hitu) {
/*JP
                            pline_The("%s hits you as you try to snatch it!",
*/
                            pline_The("%sを奪おうとしたらあなたに当たった！",
                                      the(onambuf));
                        }
                        place_object(otmp, u.ux, u.uy);
                        stackobj(otmp);
                        break;
                    }
#endif /* 0 */
                    /* right into your inventory */
/*JP
                    You("snatch %s!", yname(otmp));
*/
                    You("%sを奪った！", xname(otmp));
                    if (otmp->otyp == CORPSE
                        && touch_petrifies(&mons[otmp->corpsenm]) && !uarmg
                        && !Stone_resistance
                        && !(poly_when_stoned(youmonst.data)
                             && polymon(PM_STONE_GOLEM))) {
                        char kbuf[BUFSZ];

#if 0 /*JP*/
                        Sprintf(kbuf, "%s corpse",
                                an(mons[otmp->corpsenm].mname));
                        pline("Snatching %s is a fatal mistake.", kbuf);
#else
                        pline("%sの死体を奪ったのは致命的な間違いだ．",
                                mons[otmp->corpsenm].mname);
                        Sprintf(kbuf, "%sの死体に触れて",
                                mons[otmp->corpsenm].mname);
#endif
                        instapetrify(kbuf);
                    }
                    otmp = hold_another_object(
/*JP
                        otmp, "You drop %s!", doname(otmp), (const char *) 0);
*/
                        otmp, "%sを落した！", doname(otmp), (const char *) 0);
                    break;
                default:
                    /* to floor beneath mon */
/*JP
                    You("yank %s from %s %s!", the(onambuf),
*/
                    You("%sを%sの%sからひっぱった！", the(xname(otmp)),
                        s_suffix(mon_nam(mtmp)), mon_hand);
                    obj_no_longer_held(otmp);
                    place_object(otmp, mtmp->mx, mtmp->my);
                    stackobj(otmp);
                    break;
                }
            } else {
                pline1(msg_slipsfree);
            }
            wakeup(mtmp);
        } else {
            if (mtmp->m_ap_type && !Protection_from_shape_changers
                && !sensemon(mtmp))
                stumble_onto_mimic(mtmp);
            else
/*JP
                You("flick your bullwhip towards %s.", mon_nam(mtmp));
*/
                You("%sに向って鞭を打った．", mon_nam(mtmp));
            if (proficient) {
                if (attack(mtmp))
                    return 1;
                else
                    pline1(msg_snap);
            }
        }

    } else if (Is_airlevel(&u.uz) || Is_waterlevel(&u.uz)) {
        /* it must be air -- water checked above */
/*JP
        You("snap your whip through thin air.");
*/
        You("何もないところで鞭を打った．");

    } else {
        pline1(msg_snap);
    }
    return 1;
}

static const char
/*JP
    not_enough_room[] = "There's not enough room here to use that.",
*/
    not_enough_room[] = "それを使うだけの広さがない．",
/*JP
    where_to_hit[] = "Where do you want to hit?",
*/
    where_to_hit[] = "どれを狙う？",
/*JP
    cant_see_spot[] = "won't hit anything if you can't see that spot.",
*/
    cant_see_spot[] = "場所が見えなければ狙えない．",
/*JP
    cant_reach[] = "can't reach that spot from here.";
*/
    cant_reach[] = "ここからそこへは届かない．";

/* find pos of monster in range, if only one monster */
boolean
find_poleable_mon(pos, min_range, max_range)
coord *pos;
int min_range, max_range;
{
    struct monst *mtmp;
    struct monst *selmon = (struct monst *) 0;

    for (mtmp = fmon; mtmp; mtmp = mtmp->nmon)
        if (mtmp && !DEADMONSTER(mtmp) && !mtmp->mtame
            && cansee(mtmp->mx, mtmp->my)
            && distu(mtmp->mx, mtmp->my) <= max_range
            && distu(mtmp->mx, mtmp->my) >= min_range) {
            if (selmon)
                return FALSE;
            selmon = mtmp;
        }
    if (!selmon)
        return FALSE;
    pos->x = selmon->mx;
    pos->y = selmon->my;
    return TRUE;
}

static int polearm_range_min = -1;
static int polearm_range_max = -1;

void
display_polearm_positions(state)
int state;
{
    if (state == 0) {
        tmp_at(DISP_BEAM, cmap_to_glyph(S_goodpos));
    } else if (state == 1) {
        int x, y, dx, dy;

        for (dx = -4; dx <= 4; dx++)
            for (dy = -4; dy <= 4; dy++) {
                x = dx + (int) u.ux;
                y = dy + (int) u.uy;
                if (isok(x, y) && ACCESSIBLE(levl[x][y].typ)
                    && distu(x, y) >= polearm_range_min
                    && distu(x, y) <= polearm_range_max) {
                    tmp_at(x, y);
                }
            }
    } else {
        tmp_at(DISP_END, 0);
    }
}

/* Distance attacks by pole-weapons */
STATIC_OVL int
use_pole(obj)
struct obj *obj;
{
    int res = 0, typ, max_range, min_range, glyph;
    coord cc;
    struct monst *mtmp;
    struct monst *hitm = context.polearm.hitmon;

    /* Are you allowed to use the pole? */
    if (u.uswallow) {
        pline(not_enough_room);
        return 0;
    }
    if (obj != uwep) {
        if (!wield_tool(obj, "swing"))
            return 0;
        else
            res = 1;
    }
    /* assert(obj == uwep); */

    /*
     * Calculate allowable range (pole's reach is always 2 steps):
     *  unskilled and basic: orthogonal direction, 4..4;
     *  skilled: as basic, plus knight's jump position, 4..5;
     *  expert: as skilled, plus diagonal, 4..8.
     *      ...9...
     *      .85458.
     *      .52125.
     *      9410149
     *      .52125.
     *      .85458.
     *      ...9...
     *  (Note: no roles in nethack can become expert or better
     *  for polearm skill; Yeoman in slash'em can become expert.)
     */
    min_range = 4;
    typ = uwep_skill_type();
    if (typ == P_NONE || P_SKILL(typ) <= P_BASIC)
        max_range = 4;
    else if (P_SKILL(typ) == P_SKILLED)
        max_range = 5;
    else
        max_range = 8; /* (P_SKILL(typ) >= P_EXPERT) */

    polearm_range_min = min_range;
    polearm_range_max = max_range;

    /* Prompt for a location */
    pline(where_to_hit);
    cc.x = u.ux;
    cc.y = u.uy;
    if (!find_poleable_mon(&cc, min_range, max_range) && hitm
        && !DEADMONSTER(hitm) && cansee(hitm->mx, hitm->my)
        && distu(hitm->mx, hitm->my) <= max_range
        && distu(hitm->mx, hitm->my) >= min_range) {
        cc.x = hitm->mx;
        cc.y = hitm->my;
    }
    getpos_sethilite(display_polearm_positions);
/*JP
    if (getpos(&cc, TRUE, "the spot to hit") < 0)
*/
    if (getpos(&cc, TRUE, "狙う場所") < 0)
        return res; /* ESC; uses turn iff polearm became wielded */

    glyph = glyph_at(cc.x, cc.y);
    if (distu(cc.x, cc.y) > max_range) {
/*JP
        pline("Too far!");
*/
        pline("遠すぎる！");
        return res;
    } else if (distu(cc.x, cc.y) < min_range) {
/*JP
        pline("Too close!");
*/
        pline("近すぎる！");
        return res;
    } else if (!cansee(cc.x, cc.y) && !glyph_is_monster(glyph)
               && !glyph_is_invisible(glyph) && !glyph_is_statue(glyph)) {
        You(cant_see_spot);
        return res;
    } else if (!couldsee(cc.x, cc.y)) { /* Eyes of the Overworld */
        You(cant_reach);
        return res;
    }

    context.polearm.hitmon = NULL;
    /* Attack the monster there */
    bhitpos = cc;
    if ((mtmp = m_at(bhitpos.x, bhitpos.y)) != (struct monst *) 0) {
        if (attack_checks(mtmp, uwep))
            return res;
        if (overexertion())
            return 1; /* burn nutrition; maybe pass out */
        context.polearm.hitmon = mtmp;
        check_caitiff(mtmp);
        notonhead = (bhitpos.x != mtmp->mx || bhitpos.y != mtmp->my);
        (void) thitmonst(mtmp, uwep);
    } else if (glyph_is_statue(glyph) /* might be hallucinatory */
               && sobj_at(STATUE, bhitpos.x, bhitpos.y)) {
        struct trap *t = t_at(bhitpos.x, bhitpos.y);

        if (t && t->ttyp == STATUE_TRAP
            && activate_statue_trap(t, t->tx, t->ty, FALSE)) {
            ; /* feedback has been give by animate_statue() */
        } else {
            /* Since statues look like monsters now, we say something
               different from "you miss" or "there's nobody there".
               Note:  we only do this when a statue is displayed here,
               because the player is probably attempting to attack it;
               other statues obscured by anything are just ignored. */
/*JP
            pline("Thump!  Your blow bounces harmlessly off the statue.");
*/
            pline("ゴツン！彫像は傷つかなかった．");
            wake_nearto(bhitpos.x, bhitpos.y, 25);
        }
    } else {
        /* no monster here and no statue seen or remembered here */
        if (glyph_is_invisible(glyph)) {
            /* now you know that nothing is there... */
            unmap_object(bhitpos.x, bhitpos.y);
            newsym(bhitpos.x, bhitpos.y);
        }
/*JP
        You("miss; there is no one there to hit.");
*/
        You("外れた．そこには何もない．");
    }
    u_wipe_engr(2); /* same as for melee or throwing */
    return 1;
}

STATIC_OVL int
use_cream_pie(obj)
struct obj *obj;
{
    boolean wasblind = Blind;
    boolean wascreamed = u.ucreamed;
    boolean several = FALSE;

    if (obj->quan > 1L) {
        several = TRUE;
        obj = splitobj(obj, 1L);
    }
    if (Hallucination)
/*JP
        You("give yourself a facial.");
*/
        You("クリームパックをした．");
    else
#if 0 /*JP*/
        pline("You immerse your %s in %s%s.", body_part(FACE),
              several ? "one of " : "",
              several ? makeplural(the(xname(obj))) : the(xname(obj)));
#else
        pline("%s%sに%sを沈めた．",
              xname(obj),
              several ? "のひとつ" : "", body_part(FACE));
#endif
    if (can_blnd((struct monst *) 0, &youmonst, AT_WEAP, obj)) {
        int blindinc = rnd(25);
        u.ucreamed += blindinc;
        make_blinded(Blinded + (long) blindinc, FALSE);
        if (!Blind || (Blind && wasblind))
#if 0 /*JP*/
            pline("There's %ssticky goop all over your %s.",
                  wascreamed ? "more " : "", body_part(FACE));
#else
            pline("やっかいなねばねばが%s全体に%sついた．",
                  body_part(FACE), wascreamed ? "さらに" : "");
#endif
        else /* Blind  && !wasblind */
/*JP
            You_cant("see through all the sticky goop on your %s.",
*/
            pline("やっかいなねばねばが%s全体について何も見えなくなった．",
                     body_part(FACE));
    }

    setnotworn(obj);
    /* useup() is appropriate, but we want costly_alteration()'s message */
    costly_alteration(obj, COST_SPLAT);
    obj_extract_self(obj);
    delobj(obj);
    return 0;
}

STATIC_OVL int
use_grapple(obj)
struct obj *obj;
{
    int res = 0, typ, max_range = 4, tohit;
    boolean save_confirm;
    coord cc;
    struct monst *mtmp;
    struct obj *otmp;

    /* Are you allowed to use the hook? */
    if (u.uswallow) {
        pline(not_enough_room);
        return 0;
    }
    if (obj != uwep) {
        if (!wield_tool(obj, "cast"))
            return 0;
        else
            res = 1;
    }
    /* assert(obj == uwep); */

    /* Prompt for a location */
    pline(where_to_hit);
    cc.x = u.ux;
    cc.y = u.uy;
/*JP
    if (getpos(&cc, TRUE, "the spot to hit") < 0)
*/
    if (getpos(&cc, TRUE, "狙う場所") < 0)
        return res; /* ESC; uses turn iff grapnel became wielded */

    /* Calculate range; unlike use_pole(), there's no minimum for range */
    typ = uwep_skill_type();
    if (typ == P_NONE || P_SKILL(typ) <= P_BASIC)
        max_range = 4;
    else if (P_SKILL(typ) == P_SKILLED)
        max_range = 5;
    else
        max_range = 8;
    if (distu(cc.x, cc.y) > max_range) {
/*JP
        pline("Too far!");
*/
        pline("遠すぎる！");
        return res;
    } else if (!cansee(cc.x, cc.y)) {
        You(cant_see_spot);
        return res;
    } else if (!couldsee(cc.x, cc.y)) { /* Eyes of the Overworld */
        You(cant_reach);
        return res;
    }

    /* What do you want to hit? */
    tohit = rn2(5);
    if (typ != P_NONE && P_SKILL(typ) >= P_SKILLED) {
        winid tmpwin = create_nhwindow(NHW_MENU);
        anything any;
        char buf[BUFSZ];
        menu_item *selected;

        any = zeroany; /* set all bits to zero */
        any.a_int = 1; /* use index+1 (cant use 0) as identifier */
        start_menu(tmpwin);
        any.a_int++;
/*JP
        Sprintf(buf, "an object on the %s", surface(cc.x, cc.y));
*/
        Sprintf(buf, "%sにある物体", surface(cc.x, cc.y));
        add_menu(tmpwin, NO_GLYPH, &any, 0, 0, ATR_NONE, buf,
                 MENU_UNSELECTED);
        any.a_int++;
#if 0 /*JP*/
        add_menu(tmpwin, NO_GLYPH, &any, 0, 0, ATR_NONE, "a monster",
                 MENU_UNSELECTED);
#else
        add_menu(tmpwin, NO_GLYPH, &any, 0, 0, ATR_NONE, "怪物",
                 MENU_UNSELECTED);
#endif
        any.a_int++;
/*JP
        Sprintf(buf, "the %s", surface(cc.x, cc.y));
*/
        Sprintf(buf, "%s", surface(cc.x, cc.y));
        add_menu(tmpwin, NO_GLYPH, &any, 0, 0, ATR_NONE, buf,
                 MENU_UNSELECTED);
/*JP
        end_menu(tmpwin, "Aim for what?");
*/
        end_menu(tmpwin, "何を狙う？");
        tohit = rn2(4);
        if (select_menu(tmpwin, PICK_ONE, &selected) > 0
            && rn2(P_SKILL(typ) > P_SKILLED ? 20 : 2))
            tohit = selected[0].item.a_int - 1;
        free((genericptr_t) selected);
        destroy_nhwindow(tmpwin);
    }

    /* possibly scuff engraving at your feet;
       any engraving at the target location is unaffected */
    if (tohit == 2 || !rn2(2))
        u_wipe_engr(rnd(2));

    /* What did you hit? */
    switch (tohit) {
    case 0: /* Trap */
        /* FIXME -- untrap needs to deal with non-adjacent traps */
        break;
    case 1: /* Object */
        if ((otmp = level.objects[cc.x][cc.y]) != 0) {
/*JP
            You("snag an object from the %s!", surface(cc.x, cc.y));
*/
            You("%sのものを引っ掛けた！", surface(cc.x, cc.y));
            (void) pickup_object(otmp, 1L, FALSE);
            /* If pickup fails, leave it alone */
            newsym(cc.x, cc.y);
            return 1;
        }
        break;
    case 2: /* Monster */
        bhitpos = cc;
        if ((mtmp = m_at(cc.x, cc.y)) == (struct monst *) 0)
            break;
        notonhead = (bhitpos.x != mtmp->mx || bhitpos.y != mtmp->my);
        save_confirm = flags.confirm;
        if (verysmall(mtmp->data) && !rn2(4)
            && enexto(&cc, u.ux, u.uy, (struct permonst *) 0)) {
            flags.confirm = FALSE;
            (void) attack_checks(mtmp, uwep);
            flags.confirm = save_confirm;
            check_caitiff(mtmp); /* despite fact there's no damage */
/*JP
            You("pull in %s!", mon_nam(mtmp));
*/
            You("%sを引っ張った！", mon_nam(mtmp));
            mtmp->mundetected = 0;
            rloc_to(mtmp, cc.x, cc.y);
            return 1;
        } else if ((!bigmonst(mtmp->data) && !strongmonst(mtmp->data))
                   || rn2(4)) {
            flags.confirm = FALSE;
            (void) attack_checks(mtmp, uwep);
            flags.confirm = save_confirm;
            check_caitiff(mtmp);
            (void) thitmonst(mtmp, uwep);
            return 1;
        }
    /* FALL THROUGH */
    case 3: /* Surface */
        if (IS_AIR(levl[cc.x][cc.y].typ) || is_pool(cc.x, cc.y))
/*JP
            pline_The("hook slices through the %s.", surface(cc.x, cc.y));
*/
            pline("フックは%sをさっと切った．", surface(cc.x, cc.y));
        else {
/*JP
            You("are yanked toward the %s!", surface(cc.x, cc.y));
*/
            You("%sへ引っぱられた！", surface(cc.x, cc.y));
            hurtle(sgn(cc.x - u.ux), sgn(cc.y - u.uy), 1, FALSE);
            spoteffects(TRUE);
        }
        return 1;
    default: /* Yourself (oops!) */
        if (P_SKILL(typ) <= P_BASIC) {
/*JP
            You("hook yourself!");
*/
            You("自分自身を引っ掛けた！");
/*JP
            losehp(Maybe_Half_Phys(rn1(10, 10)), "a grappling hook",
*/
            losehp(Maybe_Half_Phys(rn1(10, 10)), "自分自身を引っ掛けて",
                   KILLED_BY);
            return 1;
        }
        break;
    }
    pline1(nothing_happens);
    return 1;
}

#define BY_OBJECT ((struct monst *) 0)

/* return 1 if the wand is broken, hence some time elapsed */
STATIC_OVL int
do_break_wand(obj)
struct obj *obj;
{
/*JP
    static const char nothing_else_happens[] = "But nothing else happens...";
*/
    static const char nothing_else_happens[] = "しかし，何も起きなかった．．．";
    register int i, x, y;
    register struct monst *mon;
    int dmg, damage;
    boolean affects_objects;
    boolean shop_damage = FALSE;
    boolean fillmsg = FALSE;
    int expltype = EXPL_MAGICAL;
    char confirm[QBUFSZ], buf[BUFSZ];
/*JP
    boolean is_fragile = (!strcmp(OBJ_DESCR(objects[obj->otyp]), "balsa"));
*/
    boolean is_fragile = (!strcmp(OBJ_DESCR(objects[obj->otyp]), "バルサの杖"));

#if 0 /*JP*/
    if (!paranoid_query(ParanoidBreakwand,
                       safe_qbuf(confirm,
                                 "Are you really sure you want to break ",
                                 "?", obj, yname, ysimple_name, "the wand")))
#else
    if (!paranoid_query(ParanoidBreakwand,
                       safe_qbuf(confirm,
                                 "本当に", "を壊すの？",
                                 obj, xname, ysimple_name, "杖")))
#endif
        return 0;

    if (nohands(youmonst.data)) {
/*JP
        You_cant("break %s without hands!", yname(obj));
*/
        You("手が無いので%sを壊せない！", xname(obj));
        return 0;
    } else if (ACURR(A_STR) < (is_fragile ? 5 : 10)) {
/*JP
        You("don't have the strength to break %s!", yname(obj));
*/
        You("%sを壊すだけの力がない！", xname(obj));
        return 0;
    }
/*JP
    pline("Raising %s high above your %s, you break it in two!", yname(obj),
*/
    You("%sを%sの上に高く掲げ，二つにへし折った！", xname(obj),
          body_part(HEAD));

    /* [ALI] Do this first so that wand is removed from bill. Otherwise,
     * the freeinv() below also hides it from setpaid() which causes problems.
     */
    if (obj->unpaid) {
        check_unpaid(obj); /* Extra charge for use */
        costly_alteration(obj, COST_DSTROY);
    }

    current_wand = obj; /* destroy_item might reset this */
    freeinv(obj);       /* hide it from destroy_item instead... */
    setnotworn(obj);    /* so we need to do this ourselves */

    if (!zappable(obj)) {
        pline(nothing_else_happens);
        goto discard_broken_wand;
    }
    /* successful call to zappable() consumes a charge; put it back */
    obj->spe++;
    /* might have "wrested" a final charge, taking it from 0 to -1;
       if so, we just brought it back up to 0, which wouldn't do much
       below so give it 1..3 charges now, usually making it stronger
       than an ordinary last charge (the wand is already gone from
       inventory, so perm_invent can't accidentally reveal this) */
    if (!obj->spe)
        obj->spe = rnd(3);

    obj->ox = u.ux;
    obj->oy = u.uy;
    dmg = obj->spe * 4;
    affects_objects = FALSE;

    switch (obj->otyp) {
    case WAN_WISHING:
    case WAN_NOTHING:
    case WAN_LOCKING:
    case WAN_PROBING:
    case WAN_ENLIGHTENMENT:
    case WAN_OPENING:
    case WAN_SECRET_DOOR_DETECTION:
        pline(nothing_else_happens);
        goto discard_broken_wand;
    case WAN_DEATH:
    case WAN_LIGHTNING:
        dmg *= 4;
        goto wanexpl;
    case WAN_FIRE:
        expltype = EXPL_FIERY;
    case WAN_COLD:
        if (expltype == EXPL_MAGICAL)
            expltype = EXPL_FROSTY;
        dmg *= 2;
    case WAN_MAGIC_MISSILE:
    wanexpl:
        explode(u.ux, u.uy, -(obj->otyp), dmg, WAND_CLASS, expltype);
        makeknown(obj->otyp); /* explode describes the effect */
        goto discard_broken_wand;
    case WAN_STRIKING:
        /* we want this before the explosion instead of at the very end */
/*JP
        pline("A wall of force smashes down around you!");
*/
        pline("あなたは魔力の壁につつまれた！");
        dmg = d(1 + obj->spe, 6); /* normally 2d12 */
        /*FALLTHRU*/
    case WAN_CANCELLATION:
    case WAN_POLYMORPH:
    case WAN_TELEPORTATION:
    case WAN_UNDEAD_TURNING:
        affects_objects = TRUE;
        break;
    default:
        break;
    }

    /* magical explosion and its visual effect occur before specific effects
     */
    /* [TODO?  This really ought to prevent the explosion from being
       fatal so that we never leave a bones file where none of the
       surrounding targets (or underlying objects) got affected yet.] */
    explode(obj->ox, obj->oy, -(obj->otyp), rnd(dmg), WAND_CLASS,
            EXPL_MAGICAL);

    /* prepare for potential feedback from polymorph... */
    zapsetup();

    /* this makes it hit us last, so that we can see the action first */
    for (i = 0; i <= 8; i++) {
        bhitpos.x = x = obj->ox + xdir[i];
        bhitpos.y = y = obj->oy + ydir[i];
        if (!isok(x, y))
            continue;

        if (obj->otyp == WAN_DIGGING) {
            schar typ;

            if (dig_check(BY_OBJECT, FALSE, x, y)) {
                if (IS_WALL(levl[x][y].typ) || IS_DOOR(levl[x][y].typ)) {
                    /* normally, pits and holes don't anger guards, but they
                     * do if it's a wall or door that's being dug */
                    watch_dig((struct monst *) 0, x, y, TRUE);
                    if (*in_rooms(x, y, SHOPBASE))
                        shop_damage = TRUE;
                }
                /*
                 * Let liquid flow into the newly created pits.
                 * Adjust corresponding code in music.c for
                 * drum of earthquake if you alter this sequence.
                 */
                typ = fillholetyp(x, y, FALSE);
                if (typ != ROOM) {
                    levl[x][y].typ = typ;
                    liquid_flow(x, y, typ, t_at(x, y),
                                fillmsg
                                  ? (char *) 0
/*JP
                                  : "Some holes are quickly filled with %s!");
*/
                                  : "穴はすぐに%sで埋まった！");
                    fillmsg = TRUE;
                } else
                    digactualhole(x, y, BY_OBJECT, (rn2(obj->spe) < 3
                                                    || (!Can_dig_down(&u.uz)
                                                        && !levl[x][y].candig))
                                                      ? PIT
                                                      : HOLE);
            }
            continue;
        } else if (obj->otyp == WAN_CREATE_MONSTER) {
            /* u.ux,u.uy creates it near you--x,y might create it in rock */
            (void) makemon((struct permonst *) 0, u.ux, u.uy, NO_MM_FLAGS);
            continue;
        } else if (x != u.ux || y != u.uy) {
            /*
             * Wand breakage is targetting a square adjacent to the hero,
             * which might contain a monster or a pile of objects or both.
             * Handle objects last; avoids having undead turning raise an
             * undead's corpse and then attack resulting undead monster.
             * obj->bypass in bhitm() prevents the polymorphing of items
             * dropped due to monster's polymorph and prevents undead
             * turning that kills an undead from raising resulting corpse.
             */
            if ((mon = m_at(x, y)) != 0) {
                (void) bhitm(mon, obj);
                /* if (context.botl) bot(); */
            }
            if (affects_objects && level.objects[x][y]) {
                (void) bhitpile(obj, bhito, x, y, 0);
                if (context.botl)
                    bot(); /* potion effects */
            }
        } else {
            /*
             * Wand breakage is targetting the hero.  Using xdir[]+ydir[]
             * deltas for location selection causes this case to happen
             * after all the surrounding squares have been handled.
             * Process objects first, in case damage is fatal and leaves
             * bones, or teleportation sends one or more of the objects to
             * same destination as hero (lookhere/autopickup); also avoids
             * the polymorphing of gear dropped due to hero's transformation.
             * (Unlike with monsters being hit by zaps, we can't rely on use
             * of obj->bypass in the zap code to accomplish that last case
             * since it's also used by retouch_equipment() for polyself.)
             */
            if (affects_objects && level.objects[x][y]) {
                (void) bhitpile(obj, bhito, x, y, 0);
                if (context.botl)
                    bot(); /* potion effects */
            }
            damage = zapyourself(obj, FALSE);
            if (damage) {
#if 0 /*JP:T*/
                Sprintf(buf, "killed %sself by breaking a wand", uhim());
                losehp(Maybe_Half_Phys(damage), buf, NO_KILLER_PREFIX);
#else
                Strcpy(buf, "自分自身で杖を壊してダメージを受け");
                losehp(Maybe_Half_Phys(damage), buf, KILLED_BY);
#endif
            }
            if (context.botl)
                bot(); /* blindness */
        }
    }

    /* potentially give post zap/break feedback */
    zapwrapup();

    /* Note: if player fell thru, this call is a no-op.
       Damage is handled in digactualhole in that case */
    if (shop_damage)
/*JP
        pay_for_damage("dig into", FALSE);
*/
        pay_for_damage("穴をあける", FALSE);

    if (obj->otyp == WAN_LIGHT)
        litroom(TRUE, obj); /* only needs to be done once */

discard_broken_wand:
    obj = current_wand; /* [see dozap() and destroy_item()] */
    current_wand = 0;
    if (obj)
        delobj(obj);
    nomul(0);
    return 1;
}

STATIC_OVL void
add_class(cl, class)
char *cl;
char class;
{
    char tmp[2];

    tmp[0] = class;
    tmp[1] = '\0';
    Strcat(cl, tmp);
}

static const char tools[] = { TOOL_CLASS, WEAPON_CLASS, WAND_CLASS, 0 };

/* augment tools[] if various items are carried */
STATIC_OVL void
setapplyclasses(class_list)
char class_list[];
{
    register struct obj *otmp;
    int otyp;
    boolean knowoil, knowtouchstone, addpotions, addstones, addfood;

    knowoil = objects[POT_OIL].oc_name_known;
    knowtouchstone = objects[TOUCHSTONE].oc_name_known;
    addpotions = addstones = addfood = FALSE;
    for (otmp = invent; otmp; otmp = otmp->nobj) {
        otyp = otmp->otyp;
        if (otyp == POT_OIL
            || (otmp->oclass == POTION_CLASS
                && (!otmp->dknown
                    || (!knowoil && !objects[otyp].oc_name_known))))
            addpotions = TRUE;
        if (otyp == TOUCHSTONE
            || (is_graystone(otmp)
                && (!otmp->dknown
                    || (!knowtouchstone && !objects[otyp].oc_name_known))))
            addstones = TRUE;
        if (otyp == CREAM_PIE || otyp == EUCALYPTUS_LEAF)
            addfood = TRUE;
    }

    class_list[0] = '\0';
    if (addpotions || addstones)
        add_class(class_list, ALL_CLASSES);
    Strcat(class_list, tools);
    if (addpotions)
        add_class(class_list, POTION_CLASS);
    if (addstones)
        add_class(class_list, GEM_CLASS);
    if (addfood)
        add_class(class_list, FOOD_CLASS);
}

/* the 'a' command */
int
doapply()
{
    struct obj *obj;
    register int res = 1;
    char class_list[MAXOCLASSES + 2];

    if (check_capacity((char *) 0))
        return 0;

    setapplyclasses(class_list); /* tools[] */
    obj = getobj(class_list, "use or apply");
    if (!obj)
        return 0;

    if (!retouch_object(&obj, FALSE))
        return 1; /* evading your grasp costs a turn; just be
                     grateful that you don't drop it as well */

    if (obj->oclass == WAND_CLASS)
        return do_break_wand(obj);

    switch (obj->otyp) {
    case BLINDFOLD:
    case LENSES:
        if (obj == ublindf) {
            if (!cursed(obj))
                Blindf_off(obj);
        } else if (!ublindf) {
            Blindf_on(obj);
        } else {
#if 0 /*JP*/
            You("are already %s.", ublindf->otyp == TOWEL
                                       ? "covered by a towel"
                                       : ublindf->otyp == BLINDFOLD
                                             ? "wearing a blindfold"
                                             : "wearing lenses");
#else
            You("もう%sている．", ublindf->otyp == TOWEL
                                      ? "タオルを巻い"
                                      : ublindf->otyp == BLINDFOLD
                                            ? "目隠しをつけ"
                                            : "レンズをつけ");
#endif
        }
        break;
    case CREAM_PIE:
        res = use_cream_pie(obj);
        break;
    case BULLWHIP:
        res = use_whip(obj);
        break;
    case GRAPPLING_HOOK:
        res = use_grapple(obj);
        break;
    case LARGE_BOX:
    case CHEST:
    case ICE_BOX:
    case SACK:
    case BAG_OF_HOLDING:
    case OILSKIN_SACK:
        res = use_container(&obj, 1);
        break;
    case BAG_OF_TRICKS:
        (void) bagotricks(obj, FALSE, (int *) 0);
        break;
    case CAN_OF_GREASE:
        use_grease(obj);
        break;
    case LOCK_PICK:
    case CREDIT_CARD:
    case SKELETON_KEY:
        res = (pick_lock(obj) != 0);
        break;
    case PICK_AXE:
    case DWARVISH_MATTOCK:
        res = use_pick_axe(obj);
        break;
    case TINNING_KIT:
        use_tinning_kit(obj);
        break;
    case LEASH:
        use_leash(obj);
        break;
    case SADDLE:
        res = use_saddle(obj);
        break;
    case MAGIC_WHISTLE:
        use_magic_whistle(obj);
        break;
    case TIN_WHISTLE:
        use_whistle(obj);
        break;
    case EUCALYPTUS_LEAF:
        /* MRKR: Every Australian knows that a gum leaf makes an excellent
         * whistle, especially if your pet is a tame kangaroo named Skippy.
         */
        if (obj->blessed) {
            use_magic_whistle(obj);
            /* sometimes the blessing will be worn off */
            if (!rn2(49)) {
                if (!Blind) {
/*JP
                    pline("%s %s.", Yobjnam2(obj, "glow"), hcolor("brown"));
*/
                    pline("%sは%s輝いた．", xname(obj), jconj_adj(hcolor("茶色の")));
                    obj->bknown = 1;
                }
                unbless(obj);
            }
        } else {
            use_whistle(obj);
        }
        break;
    case STETHOSCOPE:
        res = use_stethoscope(obj);
        break;
    case MIRROR:
        res = use_mirror(obj);
        break;
    case BELL:
    case BELL_OF_OPENING:
        use_bell(&obj);
        break;
    case CANDELABRUM_OF_INVOCATION:
        use_candelabrum(obj);
        break;
    case WAX_CANDLE:
    case TALLOW_CANDLE:
        use_candle(&obj);
        break;
    case OIL_LAMP:
    case MAGIC_LAMP:
    case BRASS_LANTERN:
        use_lamp(obj);
        break;
    case POT_OIL:
        light_cocktail(obj);
        break;
    case EXPENSIVE_CAMERA:
        res = use_camera(obj);
        break;
    case TOWEL:
        res = use_towel(obj);
        break;
    case CRYSTAL_BALL:
        use_crystal_ball(&obj);
        break;
    case MAGIC_MARKER:
        res = dowrite(obj);
        break;
    case TIN_OPENER:
        if (!carrying(TIN)) {
/*JP
            You("have no tin to open.");
*/
            You("缶を持っていない．");
            goto xit;
        }
/*JP
        You("cannot open a tin without eating or discarding its contents.");
*/
        pline("中身を食べるか，捨てるかしないと缶を開けられない．");
        if (flags.verbose)
/*JP
            pline("In order to eat, use the 'e' command.");
*/
            pline("食べるには，'e'コマンドを使えばよい．");
        if (obj != uwep)
            pline(
/*JP
          "Opening the tin will be much easier if you wield the tin opener.");
*/
          "缶切りを装備していれば，ずっと簡単に開けることができる．");
        goto xit;

    case FIGURINE:
        use_figurine(&obj);
        break;
    case UNICORN_HORN:
        use_unicorn_horn(obj);
        break;
    case WOODEN_FLUTE:
    case MAGIC_FLUTE:
    case TOOLED_HORN:
    case FROST_HORN:
    case FIRE_HORN:
    case WOODEN_HARP:
    case MAGIC_HARP:
    case BUGLE:
    case LEATHER_DRUM:
    case DRUM_OF_EARTHQUAKE:
        res = do_play_instrument(obj);
        break;
    case HORN_OF_PLENTY: /* not a musical instrument */
        (void) hornoplenty(obj, FALSE);
        break;
    case LAND_MINE:
    case BEARTRAP:
        use_trap(obj);
        break;
    case FLINT:
    case LUCKSTONE:
    case LOADSTONE:
    case TOUCHSTONE:
        use_stone(obj);
        break;
    default:
        /* Pole-weapons can strike at a distance */
        if (is_pole(obj)) {
            res = use_pole(obj);
            break;
        } else if (is_pick(obj) || is_axe(obj)) {
            res = use_pick_axe(obj);
            break;
        }
/*JP
        pline("Sorry, I don't know how to use that.");
*/
        pline("それをどうやって使うんだい？");
    xit:
        nomul(0);
        return 0;
    }
    if (res && obj && obj->oartifact)
        arti_speak(obj);
    nomul(0);
    return res;
}

/* Keep track of unfixable troubles for purposes of messages saying you feel
 * great.
 */
int
unfixable_trouble_count(is_horn)
boolean is_horn;
{
    int unfixable_trbl = 0;

    if (Stoned)
        unfixable_trbl++;
    if (Strangled)
        unfixable_trbl++;
    if (Wounded_legs && !u.usteed)
        unfixable_trbl++;
    if (Slimed)
        unfixable_trbl++;
    /* lycanthropy is undesirable, but it doesn't actually make you feel bad */

    if (!is_horn || (Confusion & ~TIMEOUT))
        unfixable_trbl++;
    if (!is_horn || (Sick & ~TIMEOUT))
        unfixable_trbl++;
    if (!is_horn || (HHallucination & ~TIMEOUT))
        unfixable_trbl++;
    if (!is_horn || (Vomiting & ~TIMEOUT))
        unfixable_trbl++;
    if (!is_horn || (HStun & ~TIMEOUT))
        unfixable_trbl++;
    if (!is_horn || (HDeaf & ~TIMEOUT))
        unfixable_trbl++;

    return unfixable_trbl;
}

/*apply.c*/

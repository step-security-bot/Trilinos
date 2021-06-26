C    Copyright(C) 1999-2020 National Technology & Engineering Solutions
C    of Sandia, LLC (NTESS).  Under the terms of Contract DE-NA0003525 with
C    NTESS, the U.S. Government retains certain rights in this software.
C
C    See packages/seacas/LICENSE for details

      SUBROUTINE PAVING (NBNODE, NPRM, MLN, IPTPER, NUMPER, LPERIM,
     &   XN, YN, ZN, IEXK, INXE, NNN, LLL, KKK, MXND, ANGLE,
     &   BNSIZE, LNODES, LINKPR, NPERIM, LXK, KXL, NXL, LXN, NUID,
     &   IAVAIL, NAVAIL, GRAPH, TIMER, VIDEO, DEFSIZ, SIZEIT, DEV1,
     &   KREG, BATCH, NOROOM, ERR, AMESUR, XNOLD, YNOLD, NXKOLD,
     &   MMPOLD, LINKEG, LISTEG, BMESUR, MLINK, NPROLD, NPNOLD, NPEOLD,
     &   NNXK, REMESH, REXMIN, REXMAX, REYMIN, REYMAX, IDIVIS, SIZMIN,
     &   EMAX, EMIN)
C***********************************************************************

C  SUBROUTINE PAVING = A SUBROUTINE TO PAVE A REGION GIVEN THE INITIAL
C                      BOUNDARY AS A LIST OF NODES.

C***********************************************************************

C  EXTERNAL VARIABLES:

C     NBNODE = NUMBER OF NODES ON THE INITIAL BOUNDARY
C     NPRM   = NUMBER OF SEPARATE PERIMETERS IN THE BOUNDARY
C              (THERE IS ONE OUTSIDE PERIMETER AND ONE PERIMETER FOR
C              EACH HOLE IN THE BOUNDARY)
C     MLN    = NUMBER OF ATTRIBUTES NEEDED IN LNODES ARRAY.  THIS
C              NUMBER SHOULD BE PASSED IN AS EIGHT (8) CURRENTLY.
C     IPTPER = INTEGER ARRAY OF POINTERS INTO THE BNODE ARRAY.
C              EACH POINTER INDICATES THE BEGINNING NODE FOR THAT
C              PERIMETER IN LPERIM
C     NUMPER = INTEGER ARRAY CONTAINING THE NUMBER OF NODES IN EACH
C              OF THE PERIMETERS
C     LPERIM = LIST OF PERIMETER NODES
C     X      = REAL ARRAY OF X VALUES OF NODES DIMENSIONED TO MXND
C     Y      = REAL ARRAY OF Y VALUES OF NODES DIMENSIONED TO MXND
C     Z      = REAL ARRAY OF Z VALUES OF NODES DIMENSIONED TO MXND
C     IEXK   = INTEGER ARRAY OF EDGES ATTACHED TO EACH ELEMENT
C              DIMENSIONED AS (4, MXND)
C     INXE   = INTEGER ARRAY OF NODES ATTACHED TO EACH EDGE
C            = DIMENSIONED AS (2, MXND)
C     NNODE  = NUMBER OF NODES IN THE FINAL MESH
C     NEDGE  = NUMBER OF EDGES IN THE FINAL MESH
C     NELEM  = NUMBER OF ELEMENTS IN THE FINAL MESH
C     MAXND  = MAXIMUM NUMBER OF NODES EXPECTED IN THE MESH
C              (IF THIS IS EXCEEDED, NOROOM IS RETURNED AS .TRUE.)
C     RWORK1 = REAL ARRAY FOR WORKING SPACE IN PAVING - DIMENSIONED
C              TO (MXND) - THIS BECOMES THE ANGLE ARRAY
C     RWORK2 = REAL ARRAY FOR WORKING SPACE IN PAVING - DIMENSIONED
C              TO (MXND * 2) - THIS BECOMES THE BNSIZE ARRAY
C     IWORK3 = INTEGER ARRAY FOR WORKING SPACE IN PAVING - DIMENSIONED
C              TO (MXND * 8) - THIS BECOMES THE LNODES ARRAY
C     IWORK4 = INTEGER ARRAY FOR WORKING SPACE IN PAVING - DIMENSIONED
C              TO (NPRM * 3) - THIS BECOMES THE LINKPR ARRAY
C     IWORK5 = INTEGER ARRAY FOR WORKING SPACE IN PAVING - DIMENSIONED
C              TO (NPRM) - THIS BECOMES THE NPERIM ARRAY
C     IWORK6 = INTEGER ARRAY FOR WORKING SPACE IN PAVING - DIMENSIONED
C              TO (MXND * 4) - THIS BECOMES THE LXK ARRAY
C     IWORK7 = INTEGER ARRAY FOR WORKING SPACE IN PAVING - DIMENSIONED
C              TO (MXND * 6) - THIS BECOMES THE KXL ARRAY
C     IWORK8 = INTEGER ARRAY FOR WORKING SPACE IN PAVING - DIMENSIONED
C              TO (MXND * 6) - THIS BECOMES THE NXL ARRAY
C     IWORK9 = INTEGER ARRAY FOR WORKING SPACE IN PAVING - DIMENSIONED
C              TO (MXND * 4) - THIS BECOMES THE LXN ARRAY
C     IWORK10 = INTEGER ARRAY FOR WORKING SPACE IN PAVING - DIMENSIONED
C              TO (MXND) - THIS BECOMES THE NUID ARRAY
C     IDUM1  = A DUMMY INTEGER PARAMETER NEEDED BY FASTQ -  THIS BECOMES
C              IAVAIL
C     IDUM2  = A DUMMY INTEGER PARAMETER NEEDED BY FASTQ -  THIS BECOMES
C              NAVAIL
C     GRAPH  = .TRUE. IF PLOTTING AT EACH STAGE IS DESIRED
C     TIMER  = .TRUE. IF A TIMING REPORT IS DESIRED
C     VIDEO  = .TRUE. IF A VIDEO ANIMATION SEQUENCE PLOT IS DESIRED
C     DEFSIZ = THE DEFAULT SIZE OF THE ELEMENTS IN THIS REGION
C              (SET IT TO ZERO IF YOU DON'T KNOW WHAT ELSE TO DO.)
C     SIZEIT = .TRUE. IF A SIZING FUNCTION IS TO BE USED WITH PAVING
C     DEV1   = A CHARACTER VARIABLE OF LENGTH 3 DESCRIBING THE
C              PLOTTING DEVICE BEING USED.
C     KREG   = THE REGION NUMBER BEING PROCESSED (FOR PLOTTING ID)
C     BATCH  = .TRUE. IF THE PROGRAM IS BEING RUN WITHOUT
C              GRAPHICS CAPABILITIES
C     NOROOM = .TRUE. IF ARRAY SIZES BUILT ACCORDING TO MAXND ARE
C              EXCEEDED (MORE SPACE IS NEEDED)
C     ERR    = .TRUE. IF AN ERROR OCCURS DURING PAVING OR IF NOROOM
C              IS .TRUE.
C     AMESUR = THE NODAL ERROR MEASURE VARIABLE (USED IN ADAPTIVE MESHING)
C     XNOLD  = THE OLD XN ARRAY FOR THE OLD MESH (USED IN ADAPTIVE MESHING)
C     YNOLD  = THE OLD YN ARRAY FOR THE OLD MESH (USED IN ADAPTIVE MESHING)
C     NXKOLD = THE OLD CONNECTIVITY ARRAY OF THE OLD MESH
C              (USED IN ADAPTIVE MESHING)
C     MMPOLD = THE OLD MATERIAL MAP ARRAY (USED IN ADAPTIVE MESHING)
C     LINKEG = THE LINKING ARRAY MAPPING ELEMENTS TO A SEARCH GRID
C     LISTEG = THE LIST OF ELEMENT THAT THE LINK POINTS TO
C     MLINK  = THE MAXIMUM SPACE NEEDED FOR THE SEARCH GRID LINK (LINKEG)
C     NPROLD = THE NUMBER OF PROCESSED REGIONS IN THE OLD MESH
C     NPNOLD = THE NUMBER OF PROCESSED NODES IN THE OLD MESH
C     NPEOLD = THE NUMBER OF PROCESSED ELEMENTS IN THE OLD MESH
C     NNXK   = THE NUMBER OF NODES PER ELEMENT IN THE OLD MESH
C     REMESH = .TRUE. IF AN ADAPTIVE MESHING IS REQUESTED
C     REXMIN = MIN X FOR THE OLD MESH
C     REXMAX = MAX X FOR THE OLD MESH
C     REYMIN = MIN Y FOR THE OLD MESH
C     REYMAX = MAX Y FOR THE OLD MESH
C     IDIVIS = NUMBER OF DIVISIONS IN THE SEARCH GRID LINK

C***********************************************************************

C  INTERNAL VARIABLES:
C     ANGLE  = ARRAY OF REALS FOR STORING BOUNDARY NODE ANGLES.
C     BNSIZE = ARRAY OF REALS FOR STORING ELEMENT SIZE PROPAGATION INFO.
C     LNODES = ARRAY OF INTEGERS FOR STORING BOUNDARY NODE INFORMATION.
C              IN THE LNODES ARRAY,
C              THE CORNER STATUS IS STORED IN LNODES (1, N1):
C                  0 = NOT DECIDED
C                  1 = ROW END
C                  3 = ROW SIDE
C                  5 = ROW CORNER
C                  7 = ROW REVERSAL
C              THE PRECEDING NODE IN LNODES (2, N1),
C              THE NEXT NODE IN LNODES (3, N1),
C              THE INTERIOR/EXTERIOR STATUS OF NODE IS IN LNODES (4, N1).
C                  1 = EXTERIOR OR ON THE BOUNDARY OF THE MESH
C                      (NEGATED FOR SMOOTHING)
C                  2 = INTERIOR TO THE MESH (NEGATED FOR SMOOTHING)
C              THE NEXT COUNTERCLOCKWISE LINE IS STORED IN LNODES (5, N1).
C              THE ANGLE STATUS OF LNODES IS STORED IN (6, N1),
C                  1 = ROW END ONLY
C                  2 = ROW END OR SIDE
C                  3 = ROW SIDE ONLY
C                  4 = ROW SIDE OR ROW CORNER
C                  5 = ROW CORNER ONLY
C                  6 = ROW CORNER OR REVERSAL
C                  7 = ROW REVERSAL ONLY
C              THE NUMBER OF NODES TO THE NEXT CORNER IS STORED IN (7, N1).
C              THE DEPTH OF THE ROW OF THIS NODE IS STORED IN (8, N1)
C     LINKPR = ARRAY FOR STORING LINKS TO PERIMETERS.
C     LXK    = LINES PER ELEMENT
C     KXL    = ELEMENTS PER LINE
C     NXL    = NODES PER LINE
C     LXN    = LINES PER NODE
C              NOTE:
C                 FOR *XN TABLES A NEGATIVE FLAG IN THE FOURTH COLUMN MEANS
C                 GO TO THAT ROW FOR A CONTINUATION OF THE LIST.  IN THAT ROW
C                 THE FIRST ELEMENT WILL BE NEGATED TO INDICATE THAT THIS IS
C                 A CONTINUATION ROW.
C                 A NEGATIVE FLAG IN THE SECOND COLUMN OF THE LXN ARRAY MEANS
C                 THAT THIS NODE IS AN EXTERIOR BOUNDARY NODE.

C***********************************************************************

      COMMON /TIMING/ TIMEA, TIMEP, TIMEC, TIMEPC, TIMEAJ, TIMES

      PARAMETER (MXLOOP = 20)
      PARAMETER (MXCORN = 10)
      PARAMETER (MXPICK = 1024)

C  MXPICK MUST BE SET AT (2 ** MXCORN)

      DIMENSION ICOMB (MXCORN, MXPICK), ITYPE (MXPICK)

      DIMENSION ANGLE (MXND), BNSIZE (2, MXND), LNODES (MLN, MXND)
      DIMENSION LINKPR (3, NPRM), NPERIM (NPRM)
      DIMENSION IPTPER (NPRM), NUMPER (NPRM), LPERIM(NBNODE)
      DIMENSION IEXK (4, MXND), INXE (2, 3*MXND)

      DIMENSION LCORN (MXCORN)
      DIMENSION NLOOP (MXLOOP), NEXTN1 (MXLOOP)

      DIMENSION XN(MXND), YN(MXND), ZN(MXND), NUID(MXND)
      DIMENSION LXK(4, MXND), KXL(2, 3*MXND)
      DIMENSION NXL(2, 3*MXND), LXN(4, MXND)

      DIMENSION AMESUR(NPEOLD), XNOLD(NPNOLD), YNOLD(NPNOLD)
      DIMENSION NXKOLD(NNXK, NPEOLD), MMPOLD(3, NPROLD)
      DIMENSION LINKEG(2, MLINK), LISTEG(2 * NPEOLD), BMESUR(NPNOLD)

      LOGICAL ERR, DONE, GRAPH, NOROOM, VIDEO, ADJTED
      LOGICAL SIZEIT, TIMER, CPUBRK, BATCH, REMESH

      CHARACTER*3 DEV1

      IF (REMESH) SIZEIT = .TRUE.

      TIMEA = 0.
      TIMEP = 0.
      TIMEC = 0.
      TIMEPC = 0.
      TIMEAJ = 0.
      TIMES = 0.
      CALL GETIME (TIME1)

      ERR = .FALSE.
      DONE = .FALSE.

C  ZERO ALL THE LINK ARRAYS

      DO 110 I = 1, MXND
         DO 100 J = 1, 4
            LXK (J, I) = 0
            LXN (J, I) = 0
  100    CONTINUE
  110 CONTINUE
      DO 120 I = NNN + 1, MXND
         NUID(I) = 0
  120 CONTINUE
      DO 140 I = 1, 3*MXND
         DO 130 J = 1, 2
            KXL (J, I) = 0
            NXL (J, I) = 0
  130    CONTINUE
  140 CONTINUE

C  ZERO THE LOOP COUNTING AND CONNECTING ARRAYS

      DO 150 I = 1, MXLOOP
         NLOOP (I) = 0
         NEXTN1 (I) = 0
  150 CONTINUE
      DO 160 I = 1, NPRM
         LINKPR (1, I) = 0
         LINKPR (2, I) = 0
         LINKPR (3, I) = 0
  160 CONTINUE

C  FIND THE EXTREMES OF THE PERIMETERS

      XMIN = XN (IPTPER (1))
      XMAX = XN (IPTPER (1))
      YMIN = YN (IPTPER (1))
      YMAX = YN (IPTPER (1))
      ZMIN = ZN (IPTPER (1))
      ZMAX = ZN (IPTPER (1))
      DO 180 I = 1, NPRM
         DO 170 J = IPTPER (I), IPTPER (I) + NUMPER(I) - 1
            NODE1 = LPERIM (J)
            XMIN = MIN (XN (NODE1), XMIN)
            XMAX = MAX (XN (NODE1), XMAX)
            YMIN = MIN (YN (NODE1), YMIN)
            YMAX = MAX (YN (NODE1), YMAX)
            ZMIN = MIN (ZN (NODE1), ZMIN)
            ZMAX = MAX (ZN (NODE1), ZMAX)
  170    CONTINUE
  180 CONTINUE

C  LINK ALL THE NODES IN THE ORIGINAL PERIMETERS TOGETHER

      DO 190 I = 1, NPRM
         CALL PERIML (NBNODE, MXND, NUMPER(I), IPTPER (I), MLN,
     &      XN, YN, ZN, LXK, KXL, NXL, LXN, ANGLE, BNSIZE, LNODES,
     &      LPERIM, LLL, LLLOLD, XMIN, XMAX, YMIN, YMAX, ZMIN, ZMAX,
     &      DEV1, KREG, ERR)
         IF (ERR) GOTO 310
         LINKPR (1, I) = LPERIM( IPTPER (I))
         IF (I .GT. 1) THEN
            LINKPR (2, I - 1) = I
            LINKPR (2, I) = 1
         ELSE
            LINKPR (2, I) = 0
         ENDIF
         LINKPR (3, I) = NUMPER(I)
         NPERIM (1) = NUMPER (I)
  190 CONTINUE
      ITNPER = NBNODE

C  LINK UP THE REST OF THE LXN ARRAY

      NNNOLD = NNN
      LLLOLD = LLL
      IAVAIL = NNN + 1
      NAVAIL = MXND - NNN
      DO 200 I = IAVAIL, MXND
         LXN (1, I) = 0
         LXN (2, I) = 0
         LXN (3, I) = 0
         LXN (4, I) = I + 1
  200 CONTINUE

C  PLOT THE INITIAL BOUNDARIES

      IF (GRAPH) THEN
         CALL RPLOTL (MXND, XN, YN, ZN, NXL, XMIN, XMAX, YMIN, YMAX,
     &      ZMIN, ZMAX, LLL, DEV1, KREG)
      ENDIF
      XMIN1 = XMIN
      XMAX1 = XMAX
      YMIN1 = YMIN
      YMAX1 = YMAX
      ZMIN1 = ZMIN
      ZMAX1 = ZMAX

C  CHECK INPUT FOR ODDNESS

      IF (2* (ITNPER/2) .NE. ITNPER) THEN
         CALL MESSAGE('IN PAVING, NO. OF PERIMETER NODES IS ODD')
         ERR = .TRUE.
         GOTO 310
      ENDIF

C  NOW BEGIN TO LOOP THROUGH THE INTERIOR NODE LIST
C  FILLING ROWS WITH ELEMENTS

      N1 = LINKPR (1, 1)
      N0 = LNODES (2, N1)
      KLOOP = 1
      KPERIM = 1
      NLOOP (1) = NUMPER (1)

  210 CONTINUE

C  SEE IF IT IS TIME TO SWITCH TO THE NEXT PERIMETER
C  BY WHETHER THE CURRENT N0 IS INTERIOR OR NOT

      IF (IABS (LNODES (4, N0)) .EQ. 2) THEN
         IF (LINKPR (2, KPERIM) .NE. 0) THEN
            LINKPR (3, KPERIM) = NLOOP (1)
            LINKPR (1, KPERIM) = N1
            KPERIM = LINKPR (2, KPERIM)
            N1 = LINKPR (1, KPERIM)
            NLOOP (1) = LINKPR (3, KPERIM)
            N0 = LNODES (2, N1)
         ELSE
            N0 = LNODES (2, N1)
         ENDIF
      ENDIF

C  NOW GET THE BEST CORNERS FOR THE NEXT ROW

      CALL GETROW (MXND, MXCORN, MXPICK, MLN, NUID, LXK, KXL,
     &   NXL, LXN, LNODES, NCORN, LCORN, BNSIZE, ANGLE, XN, YN, ZN,
     &   ICOMB, ITYPE, NLOOP (1), N1, NEND, IAVAIL, NAVAIL, LLL, KKK,
     &   NNN, GRAPH, VIDEO, XMIN, XMAX, YMIN, YMAX, ZMIN, ZMAX, DEV1,
     &   KREG, SIZEIT, LINKPR (2, KPERIM), NOROOM, ERR)
      IF ((NOROOM) .OR. (ERR)) GOTO 310

C  CHECK TO SEE IF WE ARE DONE WITH ONLY A QUAD LEFT
C  (AND THAT THE LOOP IS NOT AN INTERIOR HOLE)

      IF ((NLOOP (1) .EQ. 4) .AND. (LINKPR (2, KPERIM) .EQ. 0)) THEN
         CALL CLOSE4 (MXND, MLN, LXK, KXL, NXL, LXN, LNODES,
     &      LNODES (2, N1), N1, LNODES (3, N1),
     &      LNODES (3, LNODES (3, N1)), KKK, ERR)
         IF (ERR) GOTO 310
         CALL FILSMO  (MXND, MLN, XN, YN, ZN, LXK, KXL, NXL, LXN,
     &      LLL, NNN, NNN, LNODES, BNSIZE, NLOOP (1), XMIN, XMAX, YMIN,
     &      YMAX, ZMIN, ZMAX, DEV1, KREG)
         IF (GRAPH) THEN
            CALL RPLOTL (MXND, XN, YN, ZN, NXL, XMIN, XMAX, YMIN, YMAX,
     &         ZMIN, ZMAX, LLL, DEV1, KREG)
         ENDIF
         GOTO 240

C  CHECK TO SEE IF WE ARE DONE WITH ONLY 6 NODES LEFT

      ELSEIF ((NLOOP (1) .EQ. 6) .AND. (LINKPR (2, KPERIM) .EQ. 0)) THEN

         CALL CLOSE6 (MXND, MXCORN, MLN, NUID, XN, YN, LXK, KXL, NXL,
     &      LXN, ANGLE, BNSIZE, LNODES, N1, NLOOP (1), KKKOLD,
     &      LLLOLD, NNNOLD, NAVAIL, IAVAIL, DONE, XMIN, XMAX, YMIN,
     &      YMAX, DEV1, LLL, KKK, NNN, LCORN, NCORN, GRAPH, VIDEO,
     &      SIZEIT, NOROOM, ERR, XNOLD, YNOLD, NXKOLD, LINKEG, LISTEG,
     &      BMESUR, MLINK, NPNOLD, NPEOLD, NNXK, REMESH, REXMIN,
     &      REXMAX, REYMIN, REYMAX, IDIVIS, SIZMIN, EMAX, EMIN)
         IF ((NOROOM) .OR. (ERR)) GOTO 310
         CALL FILSMO  (MXND, MLN, XN, YN, ZN, LXK, KXL, NXL, LXN,
     &      LLL, NNN, NNN, LNODES, BNSIZE, NLOOP (1), XMIN, XMAX, YMIN,
     &      YMAX, ZMIN, ZMAX, DEV1, KREG)
         IF (GRAPH) THEN
            CALL RPLOTL (MXND, XN, YN, ZN, NXL, XMIN, XMAX, YMIN, YMAX,
     &         ZMIN, ZMAX, LLL, DEV1, KREG)
            CALL SFLUSH
         ENDIF
         GOTO 240

      ENDIF

C  GENERATE A NEW ROW OF ELEMENTS

      CALL ADDROW (MXND, MXCORN * MXPICK, MXLOOP, MLN, NPRM, NUID, XN,
     &   YN, ZN, LXK, KXL, NXL, LXN, ANGLE, BNSIZE, LNODES, N1, NEND,
     &   NLOOP, NEXTN1, LINKPR, KPERIM, KKKOLD, LLLOLD, NNNOLD, IAVAIL,
     &   NAVAIL, XMIN, XMAX, YMIN, YMAX, ZMIN, ZMAX, DEV1, LLL, KKK,
     &   NNN, NNN2, NADJ1, NADJ2, ICOMB, KLOOP, GRAPH, VIDEO, KREG,
     &   DONE, SIZEIT, NOROOM, ERR, XNOLD, YNOLD, NXKOLD, LINKEG,
     &   LISTEG, BMESUR, MLINK, NPNOLD, NPEOLD, NNXK, REMESH, REXMIN,
     &   REXMAX, REYMIN, REYMAX, IDIVIS, SIZMIN, EMAX, EMIN)
      IF ((NOROOM) .OR. (ERR)) GOTO 310
      IF (DONE) GOTO 240

C  TRY COLLAPSING CORNERS WITH SMALL ANGLES AFTER A ROW HAS BEEN
C  COMPLETED - NOTE THAT THE ICOMB ARRAY IS SENT TO PINCH IN PLACE
C  OF THE LCORN ARRAY FOR MORE CORNER PROCESSING CAPABILITIES

  220 CONTINUE
      CALL PINCH (MXND, MXCORN * MXPICK, MLN, NUID, XN, YN, ZN, LXK,
     &   KXL, NXL, LXN, ANGLE, LNODES, BNSIZE, N1, NLOOP (1), KKKOLD,
     &   LLLOLD, NNNOLD, IAVAIL, NAVAIL, DONE, XMIN, XMAX, YMIN, YMAX,
     &   ZMIN, ZMAX, DEV1, LLL, KKK, NNN, ICOMB, NCORN, NADJ1, NADJ2,
     &   GRAPH, VIDEO, KREG, NOROOM, ERR)
      IF ((NOROOM) .OR. (ERR)) GOTO 310
      IF (DONE) GOTO 240
CC
CC  CHECK TO SEE IF ANY ISOLATED ELEMENTS HAVE BEEN FORMED AND
CC  TAKE CARE OF THEM IF THEY HAVE
CC
C      IF (NLOOP(1) .GT. 6) THEN
C         CALL ISOEL (MXND, MLN, NUID, XN, YN, ZN, LXK, KXL, NXL, LXN,
C     &      LNODES, ANGLE, BNSIZE, IAVAIL, NAVAIL, LLL, KKK, NNN, N1,
C     &      NLOOP (1), XMIN, XMAX, YMIN, YMAX, ZMIN, ZMAX, DEV1, KREG,
C     &      ISOELM, GRAPH, VIDEO, NOROOM, ERR)
C         IF ((NOROOM) .OR. (ERR)) GOTO 220
C         IF (ISOELM) GOTO 180
C      ENDIF

C  ADJUST THE NEW ROW BY TAKING TUCKS OR INSERTING WEDGES AS NEEDED

      IF ((NADJ1 .GT. 0) .AND. (NADJ2 .GT. 0) .AND. (NLOOP(1) .GT. 4))
     &   THEN
         CALL ADJROW (MXND, MLN, NUID, XN, YN, ZN, LXK, KXL, NXL, LXN,
     &      ANGLE, BNSIZE, LNODES, NLOOP (1), IAVAIL, NAVAIL, XMIN,
     &      XMAX, YMIN, YMAX, ZMIN, ZMAX, DEV1, LLL, KKK, NNN, LLLOLD,
     &      NNNOLD, N1, NADJ1, NADJ2, NNN2, GRAPH, VIDEO, KREG, DEFSIZ,
     &      ADJTED, NOROOM, ERR)
         IF ((NOROOM) .OR. (ERR)) GOTO 310
         IF (ADJTED) GOTO 220
      ENDIF

C  CHECK TO SEE IF ANY OF THE CONCURRENT PERIMETERS OVERLAP

      IF (LINKPR (2, KPERIM) .NE. 0) THEN
         LINKPR (3, KPERIM) = NLOOP (1)
         CALL PCROSS (MXND, MXCORN * MXPICK, MLN, MXLOOP, NPRM, NUID,
     &      XN, YN, ZN, LXK, KXL, NXL, LXN, ANGLE, LNODES, BNSIZE,
     &      LINKPR, KPERIM, N1, NADJ1, NADJ2, KKKOLD, LLLOLD, NNNOLD,
     &      IAVAIL, NAVAIL, DONE, XMIN, XMAX, YMIN, YMAX, ZMIN, ZMAX,
     &      DEV1, LLL, KKK, NNN, ICOMB, NCORN, NLOOP, NEXTN1, KLOOP,
     &      GRAPH, VIDEO, KREG, NOROOM, ERR)
         IF ((NOROOM) .OR. (ERR)) GOTO 310
      ENDIF

C  TRY COLLAPSING OVERLAPPING SIDES TO FORM TWO LOOPS OUT OF THE
C  CURRENT SINGLE LOOP - NOTE THAT THE ICOMB ARRAY IS SENT AS
C  WHEN CALLING PINCH IN PLACE OF THE LCORN ARRAY

  230 CONTINUE
      IF (NLOOP (1) .GT. 6) THEN
         CALL COLAPS (MXND, MXCORN * MXPICK, MLN, MXLOOP, NUID, XN,
     &      YN, ZN, LXK, KXL, NXL, LXN, ANGLE, LNODES, BNSIZE, N1,
     &      KKKOLD, LLLOLD, NNNOLD, IAVAIL, NAVAIL, DONE, XMIN,
     &      XMAX, YMIN, YMAX, ZMIN, ZMAX, DEV1, LLL, KKK, NNN, ICOMB,
     &      NCORN, NLOOP, NEXTN1, KLOOP, GRAPH, VIDEO, KREG, NOROOM,
     &      ERR)
         IF ((NOROOM) .OR. (ERR)) GOTO 310
         IF (DONE) GOTO 240
      ENDIF

C  ADJUST THE ZOOMS TO FIT THE NEW AREA

      IF ((GRAPH) .OR. (CPUBRK (.TRUE.))) THEN
         LINKPR (3, KPERIM) = NLOOP (1)
         CALL FLMNMX (MXND, MLN, NPRM, LINKPR, KPERIM, LNODES,
     &      XN, YN, NLOOP (1), N1, XMIN, XMAX, YMIN, YMAX, ERR)
         IF (ERR) GOTO 310
         CALL RPLOTL (MXND, XN, YN, ZN, NXL, XMIN, XMAX,
     &      YMIN, YMAX, ZMIN, ZMAX, LLL, DEV1, KREG)
      ENDIF
      GOTO 210

C  CHECK TO MAKE SURE THAT OTHER LOOPS ARE NOT REMAINING TO BE FILLED

  240 CONTINUE
      IF (KLOOP .GT. 1) THEN
         N1 = NEXTN1 (1)
         DO 250 I = 1, KLOOP - 1
            NLOOP (I) = NLOOP (I + 1)
            NEXTN1 (I) = NEXTN1 (I + 1)
  250    CONTINUE
         NLOOP (KLOOP) = 0
         NEXTN1 (KLOOP) = 0
         KLOOP = KLOOP - 1

C  ADJUST THE ZOOMS TO FIT THE NEW AREA

         IF (GRAPH) THEN
            CALL FLMNMX (MXND, MLN, NPRM, LINKPR, KPERIM, LNODES,
     &         XN, YN, NLOOP (1), N1, XMIN, XMAX, YMIN, YMAX, ERR)
            IF (ERR) GOTO 310
            CALL RPLOTL (MXND, XN, YN, ZN, NXL, XMIN, XMAX,
     &         YMIN, YMAX, ZMIN, ZMAX, LLL, DEV1, KREG)
         ENDIF
         DONE = .FALSE.

C  ENTER THE FILL LOOP WHERE IT CAN CHECK TO SEE IF ANY CROSSINGS
C  ALREADY EXIST IN THIS LOOP

         GOTO 230
      ENDIF

C  THE FILL HAS BEEN COMPLETED - NOW FIX UP ANY BAD SPOTS

      DO 260 I = 1, NNN
         LNODES (4, I) = IABS (LNODES (4, I))
  260 CONTINUE
      CALL FILSMO  (MXND, MLN, XN, YN, ZN, LXK, KXL, NXL, LXN,
     &   LLL, NNN, NNN, LNODES, BNSIZE, NLOOP (1), XMIN, XMAX, YMIN,
     &   YMAX, ZMIN, ZMAX, DEV1, KREG)
      CALL TRIDEL (MXND, MLN, XN, YN, ZN, NUID, LXK, KXL, NXL, LXN,
     &   NNN, LLL, KKK, NAVAIL, IAVAIL, ANGLE, LNODES, BNSIZE,
     &   NLOOP (1), DEV1, KREG, XMIN, XMAX, YMIN, YMAX, ZMIN, ZMAX,
     &   GRAPH, VIDEO, NOROOM, ERR)
      IF ((NOROOM) .OR. (ERR)) GOTO 310
      DO 270 I = 1, NNN
         LNODES (4, I) = - IABS (LNODES (4, I))
  270 CONTINUE
      CALL FILSMO  (MXND, MLN, XN, YN, ZN, LXK, KXL, NXL, LXN,
     &   LLL, NNN, NNN, LNODES, BNSIZE, NLOOP (1), XMIN, XMAX, YMIN,
     &   YMAX, ZMIN, ZMAX, DEV1, KREG)

C  SUCCESSFUL EXIT

      IF (GRAPH) THEN
         CALL RPLOTL (MXND, XN, YN, ZN, NXL, XMIN1, XMAX1, YMIN1,
     &      YMAX1, ZMIN1, ZMAX1, LLL, DEV1, KREG)
      ENDIF

      IUPPR = MIN(LLL, MXND)
      DO 300 I = 1, IUPPR
         DO 280 J = 1, 4
            IEXK (J, I) = LXK (J, I)
  280    CONTINUE
  300 CONTINUE

      IUPPR = MIN(LLL, 3*MXND)
      DO 301 i = 1, iuppr
        DO 290 J = 1, 2
          INXE (J, I) = NXL (J, I)
  290   CONTINUE
  301 CONTINUE

C  EXIT WITH ERROR

  310 CONTINUE
      IF ((ERR) .AND. (.NOT. BATCH)) THEN
         CALL RPLOTL (MXND, XN, YN, ZN, NXL, XMIN1, XMAX1, YMIN1, YMAX1,
     &      ZMIN1, ZMAX1, LLL, DEV1, KREG)
         CALL RINGBL
         CALL SFLUSH
      ENDIF

      IF (TIMER) THEN
         CALL GETIME (TIME2)
         WRITE (*, ' (A, F10.5)')'  CPU SECONDS USED: ', TIME2-TIME1
         WRITE (*, ' (A, F10.5)')'            ADDROW: ', TIMEA
         WRITE (*, ' (A, F10.5)')'             PINCH: ', TIMEP
         WRITE (*, ' (A, F10.5)')'            COLAPS: ', TIMEC
         WRITE (*, ' (A, F10.5)')'            PCROSS: ', TIMEPC
         WRITE (*, ' (A, F10.5)')'            ADJROW: ', TIMEAJ
         WRITE (*, ' (A, F10.5)')'            SMOOTH: ', TIMES
         WRITE (*, ' (A, F10.5)')'     MISCELLANEOUS: ',
     &      TIME2-TIME1-TIMEA-TIMEP-TIMEC-TIMEPC-TIMEAJ-TIMES
         WRITE (*, ' (A, F10.5)')'         %  SMOOTH: ',
     &      TIMES * 100. / (TIME2 - TIME1)
      ENDIF
      RETURN

      END

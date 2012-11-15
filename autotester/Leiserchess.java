/**
 * Copyright (c) 2012 MIT License by 6.172 Staff
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to
 * deal in the Software without restriction, including without limitation the
 * rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 **/

//import Game;

class Leiserchess { 

/*
NOTE: this started out as Khet, so some of this could be wrong! 
format of a move:

piece are transform from one state(location, type, rotation) to another
toPiece does NOT indicate the piece at toSq

toSq = mv & 0xff
frSq = (mv & 0xff00) >> 8
fromPiece =  (mv & 0x30000) >> 16 
toPiece  = (mv & 0xc0000 >> 18
fromRot   = (mv & 0x300000) >> 20
toRot  = (mv & 0xc00000) >> 22

piece types
-----------
1 = Pawn
2 = King
*/

    public int bd[];
    public int mvs[];      //history of moves
    public int ctm;        //color to move
    public int mvlist[];   //list of moves possible from a given pos
    public int mvcount;    //index into list
    public long key[];     // position hash key
    public int kloc[];     // location of King

    private static String fileLetters = "abcdefghijkl";
    private static String Pces = "-pkPK";
    private static String PTypes = "-pk";
    private static String PRTypes = "-PK-";
    private static String rots = "ruld";
    private static int lDirs[] = {1, -16, -1, 16};
    private static int dir[] = { -16, -1, 16, 1, -17, -15, 17, 15 };
    private static int WBIT = 32;
    private static int BBIT = 16;
    private static int IBIT = 48;
    private static int BS = 10;
    private static int PAWN = 4;
    private static int KING = 8;
    private static int HALF = BS / 2;
    private static int nnn = 0;

    private boolean gameOver = false;
    public  String[] notes = new String[12];

    //str[0] should correspond to rank 8 file a ie top left  
    // Don't understand this comment -cel

    private static int change[][] = {
        { 1, 3, 1, 3 },
        { 0, 2, 0, 2 },
        { 3, 1, 3, 1 },
        { 2, 0, 2, 0 }
    };

    // back of Pawn?
    private static int pawnback[][] = {
        { 1, 1, 0, 0 },
        { 0, 1, 1, 0 },
        { 0, 0, 1, 1 },
        { 1, 0, 0, 1 },
    };

    private void initBoard() {
        gameOver = false;
        for(int i = 0; i < 256; i++) bd[i] = 0;
        for (int i=0; i<32; i++)  bd[i] = IBIT;
        for (int i=(BS+2)*16; i<256; i++) bd[i] = IBIT;
        for (int r=2; r<BS+2; r++)
            for (int f=BS; f<16; f++) bd[r*16+f] = IBIT;

        // set up pawns
        for (int f=0; f<HALF; f++) {
            for (int r=0; r<HALF; r++) {
                if (f+r == HALF-1) {
                    bd[(r+2)*16+f] = BBIT | PAWN | 0;
                    bd[(r+HALF+2)*16+(f+HALF)] = WBIT | PAWN | 2;
                }
            }
        }

        // now the center pawns
        // --------------------
        bd[ (HALF-1+2)*16 + HALF-1] = BBIT | PAWN | 0;
        bd[ (HALF-1+2)*16 + HALF] = BBIT | PAWN | 1;

        bd[ (HALF+2) * 16 + (HALF-1)] = WBIT | PAWN | 3;
        bd[ (HALF+2) * 16 + HALF] =  WBIT | PAWN | 2;

        // the kings
        bd[ 2*16 ] = BBIT | KING | 2; 
        bd[ (BS+2-1) * 16 + BS - 1] = WBIT | KING | 0;

        kloc[0] = (BS+2-1) * 16 + BS - 1;
        kloc[1] = 2*16;

        ctm = 0;  //White moves first
    }


    public Leiserchess()
    {
        key = new long[4096];
        mvs = new int[4096];
        bd = new int[256];
        kloc = new int[2];

        mvlist = new int[640];
        mvcount = 0;
        ctm = 0;

        setupPosition("");
    }


    // todo: need to improve this
    private long hashPosition()
    {
        long h = 203998918981981L;
        if ((ctm & 1) == 1) h = 703998918981987L;

        for (int r = 0; r<256; r++) {
            h = 31 * h + 11 + bd[r];
        }

        // no need to hash color to move
        return h;
    }


    public void setupPosition(String opn)
    {
        initBoard();
        key[ctm] = hashPosition();
    }


    private boolean isRep()
    {
        long k = key[ctm];
        int  count = 0;

        for (int c=ctm-4; c>=0; c-=2) {
            if (k == key[c]) {
                count++;
                if (count == 2) return true;
            }
        }

        return false;
    }

    public String status() {

        if(gameOver) {
            return "mate";
        }
        if (isRep()) {
            return "draw";
        }
        return "ok";
    }	


    //attempts to make move indicated by algstr
    //returns -1 if move is illegal
    public Integer makeMove(String algstr) 
    {
        String s = null;
        int    legal = -1;
        gen();

        for(int i = 0; i < mvcount; i++) {
            if(alg(mvlist[i]).equals(algstr)) {
                //if move is in list, should be valid
                legal = imake(mvlist[i]);
                key[ctm] = hashPosition();
                if (ctm > 0 && key[ctm] == key[ctm-1]) { legal = -1; break; }
                if (ctm > 1 && key[ctm] == key[ctm-2]) { legal = -1; break; }
                mvs[ctm] = mvlist[i];
                s = algstr;
                break;
            }
        }
        return legal;
    }


    public Integer makeToSan(String mv) 
    {
        return makeMove(mv);
    }



    //converts a move to string notation
    // ---------------------------------
    public String alg(int mv) {
        String s;
        int fromSq = (mv & 0xFF00) >> 8;
        int toSq = mv & 0xFF;
        char fromFile = fileLetters.charAt(fromSq & 15);
        char toFile = fileLetters.charAt(toSq & 15);
        String fromRank = Integer.toString( (BS+2-1) - (fromSq/16) );
        String toRank = Integer.toString( (BS+2-1) - (toSq/16) );
        int   rot = (mv >> 16) & 3;

        if (fromSq == toSq) {
            if ( rot == 1 ) {
                s = "" + fromFile + fromRank + "R";
            } else if (rot == 2) {
                s = "" + fromFile + fromRank + "U";
            } else if (rot == 3) {
                s = "" + fromFile + fromRank + "L";
            } else { // player's null move
                s = "" + fromFile + fromRank + toFile + toRank;
            }
        } else {
            s = "" + fromFile + fromRank + toFile + toRank;
        }

        return s;
    }


    public String toSq(int mv) 
    {
        if (mv == 0) return "";
        String s;

        int toSq = mv & 0xFF;
        char toFile = fileLetters.charAt(toSq & 15);
        String toRank = Integer.toString((BS+2-1) - (toSq/16));

        s = "" + toFile + toRank;

        return s;
    }



    // very low level make,  does not shoot laser
    public void ll_make(int mv) {
        int f = (mv >> 8) & 0xff;  // from square
        int t = mv & 0xff;         // to square
        int r = (mv >> 16) & 3;    // rotation
        int x;

        if (mv == 0) return;   // null move

        x = bd[f];             // from piece

        if (r == 1) { x = (x & ~3) | ((x+1) & 3); }
        if (r == 2) { x = (x & ~3) | ((x+2) & 3); }
        if (r == 3) { x = (x & ~3) | ((x+3) & 3); }

        bd[f] = bd[t];
        bd[t] = x;

        if ((bd[f] & 12) == 8)  kloc[ 1 & (bd[f] >> 4) ] = f;
        if ((bd[t] & 12) == 8)  kloc[ 1 & (bd[t] >> 4) ] = t;

        ctm++;
    }


    public int imake(int mv)
    {
        if (mv == 0) return 0;


        ll_make(mv);


        int  x = shootLaser();   // returns a square


        int  victim = 0;
        mvs[ctm] = mv;

        if (x != 0) {
            victim = bd[x];
            if ((victim & 12) == 8) gameOver = true;
            bd[x] = 0;      // kills
        }
        return victim;
    }



    public long perft(int depth) 
    {
        // System.out.printf("%s\n", getBoard());
        // System.exit(0);
        int[] board = new int[256];
        int[] Lkloc = new int[2];
        System.arraycopy(bd, 0, board, 0, 256);
        System.arraycopy(kloc, 0, Lkloc, 0, 2);
        return perftHelper(board, Lkloc, depth, 0);
    }


    public long perftHelper(int[] board, int[] somekloc, int depth, int ply) 
    {
        long nodec = 0;

        if(depth == 0) return 1;

        gen(); 

        if (depth == 1) return mvcount; 

        //local state
        int[] localBoard = new int[256];
        int[] localKloc = new int[2];

        System.arraycopy(board, 0, localBoard, 0, 256);
        System.arraycopy(somekloc, 0, localKloc, 0, 2 );

        int[] localMoves = new int[260];
        int localCtm = ctm;

        //inital list of moves
        System.arraycopy(localBoard, 0, bd, 0, 256);
        System.arraycopy(localKloc, 0, kloc, 0, 2);

        //save locally, others will modify internal game stte
        for(int i = 0; i < mvcount; i++) {
            localMoves[i] = mvlist[i];
        }

        int localMoveCount = mvcount;

        for(int i = 0; i < localMoveCount; i++) {
            int mv = localMoves[i];

            gameOver = false;
            ctm = localCtm;
            System.arraycopy(localBoard, 0, bd, 0, 256);
            System.arraycopy(localKloc, 0, kloc, 0, 2 );

            imake(localMoves[i]);

            long snodec = nodec;

            if (gameOver) { 
                nodec += 1;
            } else {
                ctm = localCtm + 1;
                nodec += perftHelper(bd, kloc, depth - 1, ply+1);
            }

        }

        System.arraycopy(localBoard, 0, bd, 0, 256);        
        System.arraycopy(localKloc, 0, kloc, 0, 2 );
        ctm = localCtm;

        return nodec;
    }


    public int shootLaser() 
    {
        int fctm = 1 ^ (ctm & 1);   // move already make, thus xor needed
        int cur =  kloc[fctm];      
        int bdir = bd[cur] & 3;
        int beam[] = {-16, 1, 16, -1};

        if ( (bd[cur] >> 2) == 2 ) {
            System.out.printf("cur = %x\n", cur );
            System.out.printf("king on wrong square: \n%s\n", getBoard());
            System.exit(1);
        }


        while (true) {
            int inc = beam[bdir];
            cur += inc;
            int c = bd[cur];
            if (c == IBIT) return 0;  // ran off board edge

            if (c != 0) {
                int typ = (c >> 2) & 3;  // type of piece we hit
                int ori = c & 3;         // orientation of piece that is hit

                switch(typ) {
                    case 1 : // pawn
                        if ( pawnback[bdir][ori] == 1 ) return cur;   // hit the back of a pawn
                        bdir = change[bdir][ori];     
                        break;

                    case 2 : // king
                        return  cur;     // sorry, game over my friend!  

                    default : 
                        System.out.printf("HEY - SHOULD NOT BE HAPPENING!\n");
                        break;
                }
            } 
        }
    }


    public void gen() 
    {
        int fctm = ctm & 1;
        int fc = 48 ^ (16 << fctm);
        int dir[] = { -17, -16, -15, -1, 1, 15, 16 ,17 };
        mvcount = 0;

        for (int r=2; r<BS+2; r++) {
            for (int f=0; f<BS; f++) {
                int sq = r*16 + f;
                if ((bd[sq] & fc) != 0) {
                    int typ = (bd[sq] >> 2) & 3;
                    for (int d=0; d<8; d++) {
                        int dest = sq + dir[d];
                        if (bd[dest] == IBIT) continue;    // illegal square
                        // if (sq == lmvt && dest == lmvf) continue;  
                        mvlist[mvcount++] = (typ << 18) | sq << 8 | dest;
                    }
                    // Rotations
                    mvlist[mvcount++] = (typ << 18) | (1 << 16) | sq << 8 | sq;
                    mvlist[mvcount++] = (typ << 18) | (2 << 16) | sq << 8 | sq;
                    mvlist[mvcount++] = (typ << 18) | (3 << 16) | sq << 8 | sq;
                    if (typ == 2) { // if King, also generate null move
                        mvlist[mvcount++] = (typ << 18) | (0 << 16) | sq << 8 | sq;
                    }
                }
            }
        }
    }



    public String getBoard() {
        String board = "";
        String pd[] = { "nw", "ne", "se", "sw" };
        String kd[] = { "nn", "ee", "ss", "ww" };

        for(int r = 0; r < BS; r++) {
            board += "\n";
            for (int f = 0; f < BS; f++) {
                int sq = (r+2)*16+f;
                if (bd[sq] == 0) { board += " --"; continue; }
                int dir = bd[sq] & 3;
                int  x = (bd[sq] >> 2) & 3;
                int  c = bd[sq] & IBIT; 

                if (c == WBIT) {
                    if (x == 1) 
                        board += ' ' + pd[dir].toUpperCase();
                    else
                        board += ' ' + kd[dir].toUpperCase();
                } else {
                    if (x == 1) 
                        board += ' ' + pd[dir];
                    else
                        board += ' ' + kd[dir];
                }

            }
        }
        board += "\n";
        return board;
    }



}

import java.io.*;
import java.util.ArrayList;
import java.util.List;
import java.util.Locale;
import java.util.Scanner;

import static java.lang.Math.ceil;
import static java.lang.Math.floor;


class Sum {
    private float sum;

    public void add(float partialSum) {
        synchronized (this) {
            sum += partialSum;
        }
    }

    public float getValue() {
        return sum;
    }
}

public class MatrixMult {
    final static int noThreads = 2;

    public static void main(String[] args) {

        MatrixMult mm = new MatrixMult();
        String[] dataFiles = {"A.txt", "B.txt"};
        try {
            mm.start(dataFiles);
        } catch (FileNotFoundException e) {
            e.printStackTrace();
        }
    }


    protected void start(String[] args) throws FileNotFoundException {
        Sum s = new Sum();
        Matrix A, B, C;
        A = read(args[0]);
        B = read(args[1]);
        Pair pieces[];
        C = new Matrix(A.nrows, B.ncols);
        System.out.println("Wczytalem A:");
        print(A);

        System.out.println("\nWczytalem B:");
        print(B);

        pieces = divideMatrixToPieces(A.rows() * B.cols());
        System.out.println("Podzial indeksow: ");
        for (Pair piece : pieces) {
            System.out.printf("%d %d\n", piece.a(), piece.b());
        }

        List<Thread> threads = new ArrayList<Thread>(noThreads);
        for (int i = 0; i < noThreads; i++) {
            Thread thread = new Thread(new MultiplicationThread(A, B, C, pieces[i], s));
            thread.run();
            threads.add(thread);
        }

        for (Thread thread : threads) {
            try {
                thread.join();
            } catch (InterruptedException e) {
                e.printStackTrace();
            }
        }
        System.out.println("Macierz obliczona równolegle");
        print(C);
        System.out.println("Suma: " + s.getValue());


        Matrix D = mult(A, B);
        System.out.println("Macierz obliczona tradycyjnie");
        print(D);

    }

    private Pair[] divideMatrixToPieces(int sizeOfMatrix) {
        Pair[] pieces = new Pair[noThreads];
        int current = 0;
        int length = sizeOfMatrix / noThreads;
        for (int i = 0; i < noThreads; i++) {
            if (current >= sizeOfMatrix) {
                pieces[i] = new Pair(0, 0);
            } else {
                int min = current + length;
                if (sizeOfMatrix < min)
                    min = sizeOfMatrix;
                pieces[i] = new Pair(current, min);
                current = pieces[i].b();
            }
        }
        return pieces;
    }

    private Matrix mult(Matrix A, Matrix B) {
        Matrix C = new Matrix(A.rows(), B.cols());

        for (int r = 0; r < A.rows(); r++) {
            for (int c = 0; c < B.cols(); c++) {
                float s = 0;
                for (int k = 0; k < A.cols(); k++) {
                    s += A.get(r, k) * B.get(k, c);
                }
                C.set(r, c, s);
            }
        }

        return C;
    }

    protected Matrix read(String fname) throws FileNotFoundException {
        File f = new File(fname);
        Scanner scanner = new Scanner(f).useLocale(Locale.ENGLISH);

        int rows = scanner.nextInt();
        int cols = scanner.nextInt();
        Matrix res = new Matrix(rows, cols);


        for (int i = 0; i < res.rows(); i++) {
            for (int j = 0; j < res.cols(); j++) {
                res.set(i, j, scanner.nextFloat());
            }
        }
        return res;
    }

    protected void print(Matrix m) {
        System.out.println("[");
        for (int r = 0; r < m.rows(); r++) {

            for (int c = 0; c < m.cols(); c++) {
                System.out.print(m.get(r, c));
                System.out.print(" ");
            }

            System.out.println("");
        }
        System.out.println("]");
    }


    public class Matrix {
        private int ncols;
        private int nrows;
        private float _data[][];

        public Matrix(int r, int c) {
            this.ncols = c;
            this.nrows = r;
            _data = new float[r][c];
        }

        public float get(int r, int c) {
            return _data[r][c];
        }

        public void set(int r, int c, float v) {
            _data[r][c] = v;
        }

        public int rows() {
            return nrows;
        }

        public int cols() {
            return ncols;
        }
    }

    public class Pair {
        private final int a;
        private final int b;

        public Pair(int a, int b) {
            this.a = a;
            this.b = b;
        }

        public int a() {
            return a;
        }

        public int b() {
            return b;
        }
    }

    public static class MultiplicationThread implements Runnable {
        private final Matrix matrixA;
        private final Matrix matrixB;
        private final Matrix resultsMatrix;
        private final Pair indexRange;
        private final Sum partialSum;

        private static Object sumMutex;

        static {
            sumMutex = new Object();
        }

        public MultiplicationThread(Matrix matrixA, Matrix matrixB, Matrix resultsMatrix, Pair indexRange, Sum partialSum) {
            this.matrixA = matrixA;
            this.matrixB = matrixB;
            this.resultsMatrix = resultsMatrix;
            this.indexRange = indexRange;
            this.partialSum = partialSum;
        }

        @Override
        public void run() {
           /*podczad dzielenia macierzy na drobne podmacierze
    otrzymujemy indeksy dla tablicy jednowymiarowej
    jednak korzystamy z tablicy dwuwymiarowej dlatego tak duzo
    obliczen pomiedzy indeksami
    Sa zawile ale dzialaja!!!
    */
            int jcord;
            int distance = indexRange.b - indexRange.a;
            float s;
            int startPointForA = (int) floor((float) indexRange.a / (float) matrixB.cols());
            int endPointForA = (int) ceil((float) indexRange.b / (float) matrixB.cols());
            for (int i = startPointForA; i < endPointForA; i++) {
                jcord = indexRange.a % matrixB.cols();
                for (int j = 0; j < distance; j++) {
                    s = 0;
                    for (int k = 0; k < matrixA.cols(); k++) {
                        s += matrixA.get(i, k) * matrixB.get(k, jcord);
                    }
                    resultsMatrix.set(i, jcord, s);

                    partialSum.add(s);

                    jcord++;
                    if (jcord >= matrixB.cols()) {
                        jcord = jcord % matrixB.cols();
                        i++;
                    }
                    //Koniec zawilych obliczen z indeksami
                }
            }
        }
    }
}

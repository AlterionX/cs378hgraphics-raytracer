#include <bits/stdc++.h>
#define N 3000
#define THRESHOLD 0.0

double circle[N][N];
// class Renderer {
// public:
// 	double mp[N][N];
// 	int n, m;

// 	Renderer(double m[N][N]) {
// 		for(int i=0; i<N*N; i++)
// 			mp[i/N][i%N] = m[i/N][i%N];
// 	}
// 	double get(int x, int y) {return mp[x][y];}
// };

// a -- b
// |    |
// c -- d
bool isdiff(double a, double b, double c, double d) {
	return ((abs(a-b) + abs(a-c) + abs(b-d) + abs(c-d))/4.0 > THRESHOLD);
}

double adaa(double r[N][N], int x1, int x2, int y1, int y2) {
	if(x1+1 >= x2 || y1+1 >= y2) return 0;
	// printf("%2d %2d / %2d %2d\n", x1, x2, y1, y2);
	int xs[] = {x1, (x1 + x2)/2, x2};
	int ys[] = {y1, (y1 + y2)/2, y2};
	double buf[3][3];
	// char mmm[N][N+1];
	// for(int i=0; i<N; i++) {for(int j=0; j<N; j++) mmm[i][j] = '.'; mmm[i][N] = '\0';}
	for(int i=0; i<3; i++) {
		for(int j=0; j<3; j++) {
			buf[i][j] = circle[xs[i]][ys[j]];
			// mmm[xs[i]][ys[j]] = (buf[i][j]>0.5) ? 'X' : 'O';
			// printf("%3.1f ", buf[i][j]);	
		}
		// printf("\n");
	}
		
	double avg = 0.0;
	for(int i=0; i<2; i++)
		for(int j=0; j<2; j++) {
			double val = (buf[i][j] + buf[i+1][j] + buf[i][j+1] + buf[i+1][j+1])/4.0;
			if(isdiff(buf[i][j], buf[i+1][j], buf[i][j+1], buf[i+1][j+1])) {
				val = adaa(r, xs[i], xs[i+1], ys[j], ys[j+1]);
			}
			avg += val;
		}
	avg /= 4.0;
	// for(int i=0; i<N; i++) printf("%s\n", mmm[i]);
	// printf(">>> %2d %2d / %2d %2d -> %.6f\n", x1, x2, y1, y2, avg);	
	return avg;
}
int main() {
	for(int i=0; i<N*N; i++) {
		if((i/N)*(i/N) + (i%N)*(i%N) <= N*N)
			circle[i/N][i%N] = 1.0;
		else
			circle[i/N][i%N] = 0.0;
	}

	printf("area = %.6f (check= %.6f)\n", adaa(circle, 0, N-1, 0, N-1), M_PI/4);
	return 0;
}
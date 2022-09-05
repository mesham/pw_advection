#include <sys/time.h>
#include <stdio.h>
#include <stdlib.h>
#include <omp.h>

void advect_flow_fields_c(double*, double*, double*, double*, double*, double* , double*, double*,double*, double*, double, double,int, int, int, int, int, int, int);
void advect_u_flow_field_c(double*, double*, double*, double* , double*, double*, double, double,int, int, int, int, int, int, int);
void advect_v_flow_field_c(double*, double*, double*, double* , double*, double*, double, double,int, int, int, int, int, int, int);
void advect_w_flow_field_c(double*, double*, double*, double* , double*, double*, double, double,int, int, int, int, int, int, int);
static long getEpoch();
static double getTiming(long, long);
static long long getTotalFLOPS(int, int, int, int);

void advect_th_field_c(double * sth, double * th, double * u, double * v, double * w, double * tzc1, double * tzc2,
                        double cx, double cy, int size_x, int size_y, int size_z, int start_x, int end_x, int start_y, int end_y) {
  int i, j, k, counter_loc, counter_loc_xp1, counter_loc_xm1, counter_loc_yp1, counter_loc_ym1;

  for (i=start_x;i<end_x;i++) {
    counter_loc=(i * size_y * size_z) + (size_z * start_y);
    counter_loc_xp1=((i+1) * size_y * size_z) + (size_z * start_y);
    counter_loc_xm1=((i-1) * size_y * size_z) + (size_z * start_y);
    counter_loc_yp1=(i * size_y * size_z) + (size_z * (start_y+1));
    counter_loc_ym1=(i * size_y * size_z) + (size_z * (start_y-1));
    for (j=start_y;j<end_y;j++) {
      for (k=1;k<size_z;k++) {
        counter_loc++;
        counter_loc_xp1++;
        counter_loc_xm1++;
        counter_loc_yp1++;
        counter_loc_ym1++;
        sth[counter_loc]=cx*0.5*(u[counter_loc_xm1] * th[counter_loc_xm1] - u[counter_loc] * th[counter_loc_xp1]);
        sth[counter_loc]=sth[counter_loc]+cy*0.5*(v[counter_loc_ym1] * th[counter_loc_ym1] - v[counter_loc] * th[counter_loc_yp1]);
        if (k < size_z - 1) {
          sth[counter_loc]=sth[counter_loc]+2.0*(tzc1[k]*w[counter_loc-1]*th[counter_loc-1] - tzc2[counter_loc]*w[counter_loc]*th[counter_loc+1]);
        } else {
          // Lid
          sth[counter_loc]=sth[counter_loc]+tzc1[k]*2.0*w[counter_loc-1]*th[counter_loc-1];
        }
      }
    }
  }
}

int main(int argc, char * argv[]) {
	int size_x=atoi(argv[1]), size_y=atoi(argv[2]), iterations=atoi(argv[3]), size_z=64;
	int hs=2;
	int start_x=hs, end_x=size_x+hs, start_y=hs, end_y=size_y+hs;
	int field_x=size_x+(hs*2), field_y=size_y+(hs*2);
	double * su=(double *) malloc(sizeof(double) * field_x * field_y * size_z);
	double * sv=(double *) malloc(sizeof(double) * field_x * field_y * size_z);
	double * sw=(double *) malloc(sizeof(double) * field_x * field_y * size_z);
	double * u=(double *) malloc(sizeof(double) * field_x * field_y * size_z);
	double * v=(double *) malloc(sizeof(double) * field_x * field_y * size_z);
	double * w=(double *) malloc(sizeof(double) * field_x * field_y * size_z);
	double * tzc1=(double *) malloc(sizeof(double) * size_z);
	double * tzc2=(double *) malloc(sizeof(double) * size_z);
	double * tzd1=(double *) malloc(sizeof(double) * size_z);
	double * tzd2=(double *) malloc(sizeof(double) * size_z);

	printf("Advecting over %d threads with compute domain X=%d Y=%d Z=%d, total domain size of X=%d Y=%d Z=%d\n", omp_get_max_threads(), size_x, size_y, size_z, field_x, field_y, size_z);

	long start=getEpoch();
  for (int i=0;i<iterations;i++) {
	  advect_flow_fields_c(su, sv, sw, u, v, w, tzc1, tzc2, tzd1, tzd2, 1.0, 2.0, field_x, field_y, size_z, start_x, end_x, start_y, end_y);
  }
	double overalltime=getTiming(getEpoch(), start);
  printf("Runtime is %f ms\n", overalltime);
	
	double kernelFLOPS=(getTotalFLOPS(size_x, size_y, size_z, iterations) / (overalltime / 1000)) / 1024 / 1024 / 1024;
  printf("Overall GFLOPS %.2f\n", kernelFLOPS);
	return 0;
}

void advect_flow_fields_c(double * su, double * sv, double * sw, double * u, double * v, double * w, double * tzc1, double * tzc2,
	double * tzd1, double * tzd2, double tcx, double tcy,
                            int size_x, int size_y, int size_z, int start_x, int end_x, int start_y, int end_y) {
	advect_u_flow_field_c(su, u, v, w, tzc1, tzc2, tcx, tcy, size_x, size_y, size_z, start_x, end_x, start_y, end_y);
	advect_v_flow_field_c(sv, u, v, w, tzc1, tzc2, tcx, tcy, size_x, size_y, size_z, start_x, end_x, start_y, end_y);
	advect_w_flow_field_c(sw, u, v, w, tzd1, tzd2, tcx, tcy, size_x, size_y, size_z, start_x, end_x, start_y, end_y);
}

void advect_u_flow_field_c(double * su, double * u, double * v, double * w, double * tzc1, double * tzc2, double tcx, double tcy,
                            int size_x, int size_y, int size_z, int start_x, int end_x, int start_y, int end_y) {
  int i, j, k, counter_loc, counter_loc_xp1, counter_loc_xm1, counter_loc_yp1, counter_loc_ym1, counter_loc_ym1_xp1;
#pragma omp parallel for private(i, counter_loc, counter_loc_xp1, counter_loc_xm1, counter_loc_yp1, counter_loc_ym1, j, k)
  for (i=start_x;i<end_x;i++) {
    counter_loc=(i * size_y * size_z) + (size_z * start_y);
    counter_loc_xp1=((i+1) * size_y * size_z) + (size_z * start_y);
    counter_loc_xm1=((i-1) * size_y * size_z) + (size_z * start_y);
    counter_loc_yp1=(i * size_y * size_z) + (size_z * (start_y+1));
    counter_loc_ym1=(i * size_y * size_z) + (size_z * (start_y-1));
    counter_loc_ym1_xp1=((i+1) * size_y * size_z) + (size_z * (start_y-1));
    for (j=start_y;j<end_y;j++) {
      for (k=1;k<size_z;k++) {
        counter_loc++;
        counter_loc_xp1++;
        counter_loc_xm1++;
        counter_loc_yp1++;
        counter_loc_ym1++;
        su[counter_loc]=tcx*(u[counter_loc_xm1] * (u[counter_loc] + u[counter_loc_xm1]) - u[counter_loc_xp1] * (u[counter_loc] + u[counter_loc_xp1]));
        su[counter_loc]=su[counter_loc]+tcy*(u[counter_loc_ym1] * (v[counter_loc_ym1] + v[counter_loc_ym1_xp1]) - u[counter_loc_yp1] * (v[counter_loc] + v[counter_loc_xp1]));
        if (k < size_z - 1) {
          su[counter_loc]=su[counter_loc]+(tzc1[k] * u[counter_loc-1] * (w[counter_loc-1] + w[counter_loc_xp1-1]) - tzc2[k] * u[counter_loc+1] * (w[counter_loc] + w[counter_loc_xp1]));
        } else {
          // Lid
          su[counter_loc]=su[counter_loc]+tzc1[k] * u[counter_loc-1] * (w[counter_loc-1] + w[counter_loc_xp1-1]);
        }
      }
    }
  }
}

void advect_v_flow_field_c(double * sv, double * u, double * v, double * w, double * tzc1, double * tzc2, double tcx, double tcy,
                            int size_x, int size_y, int size_z, int start_x, int end_x, int start_y, int end_y) {
  int i, j, k, counter_loc, counter_loc_xp1, counter_loc_xm1, counter_loc_yp1, counter_loc_ym1;
#pragma omp parallel for private(i, counter_loc, counter_loc_xp1, counter_loc_xm1, counter_loc_yp1, counter_loc_ym1, j, k)
  for (i=start_x;i<end_x;i++) {
    counter_loc=(i * size_y * size_z) + (size_z * start_y);
    counter_loc_xp1=((i+1) * size_y * size_z) + (size_z * start_y);
    counter_loc_xm1=((i-1) * size_y * size_z) + (size_z * start_y);
    counter_loc_yp1=(i * size_y * size_z) + (size_z * (start_y+1));
    counter_loc_ym1=(i * size_y * size_z) + (size_z * (start_y-1));
    for (j=start_y;j<end_y;j++) {
      for (k=1;k<size_z;k++) {
        counter_loc++;
        counter_loc_xp1++;
        counter_loc_xm1++;
        counter_loc_yp1++;
        counter_loc_ym1++;
        sv[counter_loc]=tcy * (v[counter_loc_ym1] * (v[counter_loc] + v[counter_loc_ym1]) - v[counter_loc_yp1] * (v[counter_loc] + v[counter_loc_yp1]));
        sv[counter_loc]=sv[counter_loc]+tcx*(v[counter_loc_xm1] * (u[counter_loc_xm1] + u[counter_loc_yp1]) - v[counter_loc_xp1] * (u[counter_loc] + u[counter_loc_yp1]));
        if (k < size_z - 1) {
          sv[counter_loc]=sv[counter_loc]+(tzc1[k] * v[counter_loc-1] * (w[counter_loc-1] + w[counter_loc_yp1-1]) - tzc2[k] * v[counter_loc+1] * (w[counter_loc] + w[counter_loc_yp1]));        } else {
          // Lid
          sv[counter_loc]=sv[counter_loc]+tzc1[k] * v[counter_loc-1] * (w[counter_loc-1] + w[counter_loc_yp1-1]);
        }
      }
    }
  }
}

void advect_w_flow_field_c(double * sw, double * u, double * v, double * w, double * tzd1, double * tzd2, double tcx, double tcy,
                            int size_x, int size_y, int size_z, int start_x, int end_x, int start_y, int end_y) {
  int i, j, k, counter_loc, counter_loc_xp1, counter_loc_xm1, counter_loc_yp1, counter_loc_ym1;
#pragma omp parallel for private(i, counter_loc, counter_loc_xp1, counter_loc_xm1, counter_loc_yp1, counter_loc_ym1, j, k)
  for (i=start_x;i<end_x;i++) {
    counter_loc=(i * size_y * size_z) + (size_z * start_y);
    counter_loc_xp1=((i+1) * size_y * size_z) + (size_z * start_y);
    counter_loc_xm1=((i-1) * size_y * size_z) + (size_z * start_y);
    counter_loc_yp1=(i * size_y * size_z) + (size_z * (start_y+1));
    counter_loc_ym1=(i * size_y * size_z) + (size_z * (start_y-1));
    for (j=start_y;j<end_y;j++) {
      for (k=1;k<size_z-1;k++) {
        counter_loc++;
        counter_loc_xp1++;
        counter_loc_xm1++;
        counter_loc_yp1++;
        counter_loc_ym1++;
        sw[counter_loc]=tzd1[k] * w[counter_loc-1] * (w[counter_loc] + w[counter_loc-1]) - tzd2[k] * w[counter_loc+1] * (w[counter_loc] + w[counter_loc+1]);
        sw[counter_loc]=sw[counter_loc]+tcx*(w[counter_loc_xm1]*(u[counter_loc] + u[counter_loc_xm1+1]) - w[counter_loc_xp1] * (u[counter_loc] + u[counter_loc+1]));
        sw[counter_loc]=sw[counter_loc]+tcy*(w[counter_loc_ym1] * (v[counter_loc_ym1] + v[counter_loc_ym1+1]) - w[counter_loc_yp1] * (v[counter_loc] + v[counter_loc+1]));
      }
    }
  }
}

static long getEpoch() {
  struct timeval tm;
  gettimeofday(&tm, NULL);
  return (tm.tv_sec * 1000000)+tm.tv_usec;
}

static double getTiming(long end_time, long start_time) {
  return (end_time - start_time) / 1.0e3 ;
}

static long long getTotalFLOPS(int x_size, int y_size, int z_size, int iterations) {
  long long total_elements_xu=x_size * y_size * (z_size-1);
  long long lid_elements=x_size * y_size;
  long long non_lid_elements=total_elements_xu-lid_elements;
  long long total_elements_w=x_size * y_size * (z_size-2);

  long long advectxu_flops=(lid_elements * 17) + (non_lid_elements * 21);
  long long advectw_flops=total_elements_w * 21;
  return ((advectxu_flops * 2) + advectw_flops) * iterations;
}


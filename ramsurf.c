#define mr 40000
#define mz 50002
#define mp 10
#define m 40
#define BUFSIZE 1024

#include <stdio.h>
#include <complex.h>
#include <math.h>
#include <assert.h>
#include <string.h>
#include <stdlib.h>
#include <setjmp.h>

#include "ramsurf.h"

#ifndef max
   #define max( a, b ) ( ((a) > (b)) ? (a) : (b) )
#endif

#ifndef min
   #define min( a, b ) ( ((a) < (b)) ? (a) : (b) )
#endif

typedef float complex fcomplex;
typedef double complex dcomplex;

static jmp_buf exception_env;

//
//     This subroutine finds a root of a polynomial of degree n > 2
//     by Laguerre's method.
//
static
void guerre(double a[m][2], int n, double z[2], double err, int const nter) {
    double az[50][2],azz[50][2],dz[2],p[2],pz[2],pzz[2],f[2],g[2],h[2];
    double amp1,amp2,rn,eps,tmp;
    eps=1.0e-20;
    rn=(double)n;


    //
    //     The coefficients of p'[z] and p''[z].
    //
    for(int i=1;i<=n;i++) {
        az[i-1][0]=i*a[i][0];
        az[i-1][1]=i*a[i][1];
    }
    for(int i=1;i<=n-1;i++) {
        azz[i-1][0]=i*az[i][0];
        azz[i-1][1]=i*az[i][1];
    }
    //
    int iter=0,jter=0;
    do {
        p[0]=a[n-1][0]+(a[n][0]*z[0]-a[n][1]*z[1]);
        p[1]=a[n-1][1]+(a[n][0]*z[1]+a[n][1]*z[0]);
        for(int i=n-1;i>=1;--i) {
            tmp=p[0];
            p[0]=a[i-1][0]+(z[0]*p[0]-z[1]*p[1]);
            p[1]=a[i-1][1]+(z[0]*p[1]+z[1]*tmp);
        }
        tmp=(p[0]*p[0]+p[1]*p[1]);
        if(sqrt(tmp)<eps) return;
        //
        pz[0]=az[n-2][0]+(az[n-1][0]*z[0]-az[n-1][1]*z[1]);
        pz[1]=az[n-2][1]+(az[n-1][0]*z[1]+az[n-1][1]*z[0]);
        for(int i=n-2;i>=1;i--) {
            tmp=pz[0];
            pz[0]=az[i-1][0]+(z[0]*pz[0]-z[1]*pz[1]);
            pz[1]=az[i-1][1]+(z[0]*pz[1]+z[1]*tmp);
        }
        //
        pzz[0]=azz[n-3][0]+(azz[n-2][0]*z[0]-azz[n-2][1]*z[1]);
        pzz[1]=azz[n-3][1]+(azz[n-2][0]*z[1]+azz[n-2][1]*z[0]);
        for(int i=n-3;i>=1;i--) {
            tmp=pzz[0];
            pzz[0]=azz[i-1][0]+(z[0]*pzz[0]-z[1]*pzz[1]);
            pzz[1]=azz[i-1][1]+(z[0]*pzz[1]+z[1]*tmp);
        }
        //
        //     The Laguerre perturbation.
        //
        double norm=(p[0]*p[0] + p[1]*p[1]);
        f[0]=(pz[0]*p[0] + pz[1]*p[1])/norm;
        f[1]=(pz[1]*p[0] - pz[0]*p[1])/norm;

        g[0]=(f[0]*f[0]-f[1]*f[1]) - (pzz[0]*p[0] + pzz[1]*p[1])/norm;
        g[1]=(f[0]*f[1]+f[1]*f[0]) - (pzz[1]*p[0] - pzz[0]*p[1])/norm;
        dcomplex H = rn*g[0] - (f[0]*f[0]-f[1]*f[1]) + I*( rn*g[1] - (f[0]*f[1]+f[1]*f[0]));
        H=csqrt((rn-1.0)*H);
        h[0]=creal(H);
        h[1]=cimag(H);
        double fph[2] = { f[0] + h[0], f[1] + h[1] };
        double fmh[2] = { f[0] - h[0], f[1] - h[1] };
        amp1=fph[0]*fph[0] + fph[1]*fph[1];
        amp2=fmh[0]*fmh[0] + fmh[1]*fmh[1];
        if(amp1>amp2) {
            dz[0]=-rn*fph[0]/amp1;
            dz[1]=+rn*fph[1]/amp1;
        }
        else {
            dz[0]=-rn*fmh[0]/amp2;
            dz[1]=+rn*fmh[1]/amp2;
        }
        //
        iter=iter+1;
        //
        //     Rotate by 90 degrees to avoid limit cycles. 
        //
        jter=jter+1;
        if(jter==10) {
            jter=1;
            tmp=dz[0];
            dz[0]=-dz[1];
            dz[1]=tmp;
        }
        z[0]+=dz[0];
        z[1]+=dz[1];
        //
        if(iter==100) {
    	    longjmp(exception_env, LAG_NOT_CON);
        }
        //
    } while ((sqrt(dz[0]*dz[0]+dz[1]*dz[1])>err) && (iter<nter));
    //
}

//
//     The root-finding subroutine. 
//
static
void fndrt(double a[m][2], int n, double z[m][2]) {
    double root[2];
    double err;
    //
    if(n==1) {
        double norm = a[1][0]*a[1][0] +  a[1][1]*a[1][1];
        z[0][0]=-(a[0][0]*a[1][0]+a[0][1]*a[1][1])/norm;
        z[0][1]=-(a[0][1]*a[1][0]-a[0][0]*a[1][1])/norm;
        return;
    }
    if(n>2) {
        //
        for(int k=n;k>=3;--k) {
            //
            //     Obtain an approximate root.
            //
            root[0]=root[1]=0.;
            err=1.0e-12;
            guerre(a,k,root,err,1000);
            //
            //     Refine the root by iterating five more times.
            //
            err=0.0;
            guerre(a,k,root,err,5);
            z[k-1][0]=root[0];
            z[k-1][1]=root[1];
            //
            //     Divide out the factor [z-root].
            //
            for(int i=k;i>=1;i--) {
                a[i-1][0]+=root[0]*a[i][0] - root[1]*a[i][1];
                a[i-1][1]+=root[0]*a[i][1] + root[1]*a[i][0];
            }
            for(int i=1;i<=k;i++) {
                a[i-1][0]=a[i][0];
                a[i-1][1]=a[i][1];
            }
            //
        }
    }
    //
    //     Solve the quadratic equation.
    //
    dcomplex A0 = a[0][0]+I*a[0][1];
    dcomplex A1 = a[1][0]+I*a[1][1];
    dcomplex A2 = a[2][0]+I*a[2][1];
    dcomplex SQRT = csqrt(A1*A1-4.0*A0*A2);
    dcomplex Z1 = 0.5*((-1)*A1+SQRT)/A2;
    dcomplex Z0 = 0.5*((-1)*A1-SQRT)/A2;
    z[1][0]=creal(Z1);
    z[1][1]=cimag(Z1);
    z[0][0]=creal(Z0);
    z[0][1]=cimag(Z0);
    //
}

//
//     Rows are interchanged for stability.
//
static
void pivot(int const n, int const i, double a[m][m][2], double b[m][2]){
    double temp[2];
    //
    int i0=i;
    double amp0=sqrt(a[i-1][i-1][0]*a[i-1][i-1][0]+a[i-1][i-1][1]*a[i-1][i-1][1]);
    for(int j=i+1;j<=n;j++) {
        double amp=sqrt(a[i-1][j-1][0]*a[i-1][j-1][0]+a[i-1][j-1][1]*a[i-1][j-1][1]);
        if(amp>amp0) {
            i0=j;
            amp0=amp;
        }
    }
    if(i0==i)
        return;
    //
    temp[0]=b[i-1][0];
    temp[1]=b[i-1][1];
    b[i-1][0]=b[i0-1][0];
    b[i-1][1]=b[i0-1][1];
    b[i0-1][0]=temp[0];
    b[i0-1][1]=temp[1];
    for(int j=i;j<=n;j++) {
        temp[0]=a[j-1][i-1][0];
        temp[1]=a[j-1][i-1][1];
        a[j-1][i-1][0]=a[j-1][i0-1][0];
        a[j-1][i-1][1]=a[j-1][i0-1][1];
        a[j-1][i0-1][0]=temp[0];
        a[j-1][i0-1][1]=temp[1];
    }
    //
}
//
//     Gaussian elimination.
//
static
void gauss(int const n, double a[m][m][2], double b[m][2]) {
    //
    //     Downward elimination.
    //
    for(int i=1;i<=n;i++) {
        if(i<n)
            pivot(n,i,a,b);
        double norm = a[i-1][i-1][0]*a[i-1][i-1][0]+a[i-1][i-1][1]*a[i-1][i-1][1];
        a[i-1][i-1][0]=(a[i-1][i-1][0])/norm;
        a[i-1][i-1][1]=-a[i-1][i-1][1]/norm;
        double tmp = b[i-1][0];
        b[i-1][0]=b[i-1][0]*a[i-1][i-1][0]-b[i-1][1]*a[i-1][i-1][1];
        b[i-1][1]=b[i-1][1]*a[i-1][i-1][0]+tmp*a[i-1][i-1][1];
        if(i<n){
            for(int j=i+1;j<=n;j++) {
                tmp = a[j-1][i-1][0];
                a[j-1][i-1][0]=a[j-1][i-1][0]*a[i-1][i-1][0]-a[j-1][i-1][1]*a[i-1][i-1][1];
                a[j-1][i-1][1]=a[j-1][i-1][1]*a[i-1][i-1][0]+tmp*a[i-1][i-1][1];
            }
            for(int k=i+1;k<=n;k++) {
                b[k-1][0]-=a[i-1][k-1][0]*b[i-1][0]-a[i-1][k-1][1]*b[i-1][1];
                b[k-1][1]-=a[i-1][k-1][1]*b[i-1][0]+a[i-1][k-1][0]*b[i-1][1];
                for(int j=i+1;j<=n;j++) {
                    a[j-1][k-1][0]-=a[i-1][k-1][0]*a[j-1][i-1][0]-a[i-1][k-1][1]*a[j-1][i-1][1];
                    a[j-1][k-1][1]-=a[i-1][k-1][1]*a[j-1][i-1][0]+a[i-1][k-1][0]*a[j-1][i-1][1];
                }
            }
        }
    }
    //
    //     Back substitution.
    //
    for(int i=n-1;i>=1;i--) {
        for(int j=i;j<n;j++) {
            b[i-1][0]-=a[j][i-1][0]*b[j][0]-a[j][i-1][1]*b[j][1];
            b[i-1][1]-=a[j][i-1][1]*b[j][0]+a[j][i-1][0]*b[j][1];
        }
    }
    //
}

//
//     The derivatives of the operator function at x=0.
//
static
void deriv(int n, float sig, double alp, 
        double dg[m][2], double dh1[m][2], double dh2[m][2], double dh3[m][2],
        double bin[m][m], double nu) {
    //
    dh1[0][0]=0;
    dh1[0][1]=sig*0.5;
    double exp1 = -0.5;
    dh2[0][0]=alp;
    dh2[0][1]=alp;
    double exp2=-1.0;
    dh3[0][0]=-2.0*nu;
    dh3[0][1]=0.;
    double exp3=-1.0;
    for(int i=1;i<n;i++) {
        dh1[i][0]=dh1[i-1][0]*exp1;
        dh1[i][1]=dh1[i-1][1]*exp1;
        exp1=exp1-1.0;
        dh2[i][0]=dh2[i-1][0]*exp2;
        dh2[i][1]=dh2[i-1][1]*exp2;
        exp2=exp2-1.0;
        dh3[i][0]=-nu*dh3[i-1][0]*exp3;
        dh3[i][1]=-nu*dh3[i-1][1]*exp3;
        exp3=exp3-1.0;
    }
    //
    dg[0][0]=1.0;
    dg[0][1]=0.0;
    dg[1][0]=dh1[0][0]+dh2[0][0]+dh3[0][0];
    dg[1][1]=dh1[0][1]+dh2[0][1]+dh3[0][1];
    for(int i=2;i<=n;i++) {
        dg[i][0]=dh1[i-1][0]+dh2[i-1][0]+dh3[i-1][0];
        dg[i][1]=dh1[i-1][1]+dh2[i-1][1]+dh3[i-1][1];
        for(int j=1;j<=i-1;j++) {
            double tmpr=dh1[j-1][0]+dh2[j-1][0]+dh3[j-1][0];
            double tmpi=dh1[j-1][1]+dh2[j-1][1]+dh3[j-1][1];
            dg[i][0]+=bin[j-1][i-1]*(tmpr*dg[i-j][0]-tmpi*dg[i-j][1]);
            dg[i][1]+=bin[j-1][i-1]*(tmpi*dg[i-j][0]+tmpr*dg[i-j][1]);
        }
    }
    //
}

//
//     The coefficients of the rational approximation.
//
static
void epade(int np, int ns, int const ip, float k0, float dr,  float pd1[mp][2],  float pd2[mp][2]) {
    //
    double dg[m][2],dh1[m][2],dh2[m][2],dh3[m][2],a[m][m][2],b[m][2];
    double  z1, bin[m][m],fact[m];
    float sig=k0*dr;
    int n=2*np;
    //
    double nu, alp;
    if(ip==1){
        nu=0.0;
        alp=0.0;
    }
    else {
        nu=1.0;
        alp=-0.25;
    }
    //
    //     The factorials.;
    //
    fact[0]=1.0;
    for(int i=1; i<n; i++)
        fact[i]=(i+1)*fact[i-1];
    //
    //     The binomial coefficients.;
    //
    for(int i=0;i<n+1;i++) {
        bin[0][i]=1.0;
        bin[i][i]=1.0;
    }
    for(int i=2;i<n+1;i++)
        for(int j=1;j<i;j++)
            bin[j][i]=bin[j-1][i-1]+bin[j][i-1];
    //
    for(int i=0;i<n;i++)
        for(int j=0;j<n;j++) {
            a[i][j][0]=0.0;
            a[i][j][1]=0.0;
        }
    //
    //     The accuracy constraints.;
    //
    deriv(n, sig, alp, dg, dh1, dh2, dh3, bin, nu);
    //
    for(int i=0;i<n; i++) {
        b[i][0]=dg[i+1][0];
        b[i][1]=dg[i+1][1];
    }
    for(int i=1;i<=n;i++) {
        if(2*i-1<=n) {
            a[2*i-2][i-1][0]=fact[i-1];
            a[2*i-2][i-1][1]=0.;
        }
        for(int j=1;j<=i;j++) {
            if(2*j<=n) {
                a[2*j-1][i-1][0]=-bin[j][i]*fact[j-1]*dg[i-j][0];
                a[2*j-1][i-1][1]=-bin[j][i]*fact[j-1]*dg[i-j][1];
            }
        }
    }
    //
    //     The stability constraints.;
    //
    if(ns>=1){
        z1=-3.0;
        b[n-1][0]=-1.0;
        b[n-1][1]=0.0;
        for(int j=1;j<=np;j++) {
            a[2*j-2][n-1][0]=pow(z1,j);
            a[2*j-2][n-1][1]=0.;
            a[2*j-1][n-1][0]=0.0;
            a[2*j-1][n-1][1]=0.0;
        }
    }
    //
    if(ns>=2){
        z1=-1.5;
        b[n-2][0]=-1.0;
        b[n-2][1]=0.0;
        for(int j=1;j<=np;j++) {
            a[2*j-2][n-2][0]=pow(z1,j);
            a[2*j-2][n-2][1]=0.;
            a[2*j-1][n-2][0]=0.0;
            a[2*j-1][n-2][1]=0.0;
        }
    }
    //
    gauss(n,a, b);
    //
    dh1[0][0]=1.0;
    dh1[0][1]=0.0;
    for(int j=1;j<=np;j++) {
        dh1[j][0]=b[2*j-2][0];
        dh1[j][1]=b[2*j-2][1];
    }
    fndrt(dh1,np,dh2);
    for(int j=0;j<np;j++){
        double norm = dh2[j][0]*dh2[j][0]+dh2[j][1]*dh2[j][1];
        pd1[j][0]=-dh2[j][0]/norm;
        pd1[j][1]=+dh2[j][1]/norm;
    }
    //
    dh1[0][0]=1.0;
    dh1[0][1]=0.0;
    for(int j=1;j<=np;j++) {
        dh1[j][0]=b[2*j-1][0];
        dh1[j][1]=b[2*j-1][1];
    }
    fndrt(dh1,np,dh2);

    for(int j=0;j<np;j++) {
        double norm = dh2[j][0]*dh2[j][0]+dh2[j][1]*dh2[j][1];
        pd2[j][0]=-dh2[j][0]/norm;
        pd2[j][1]=+dh2[j][1]/norm;
    }
    //
}
//
//     Profile reader and interpolator.
//
static
void zread( FILE* fs1, int nz, float dz, float prof[mz]) {
    char tmp[BUFSIZE];
    //
    for(int i=0;i<nz+2;i++)
        prof[i]=-1.0;
    float profi,zi;
    fscanf(fs1,"%f %f",&zi,&profi);
    fgets(tmp,BUFSIZE,fs1);
    //
    prof[0]=profi;
    int i=1.5+zi/dz;
    prof[i-1]=profi;
    int iold=i;
    while(1) {
        fscanf(fs1,"%f %f",&zi,&profi);
        fgets(tmp,BUFSIZE,fs1);
        if(zi<0.0) break;
        i=1.5+zi/dz;
        if(i == iold)i=i+1;
        prof[i-1]=profi;
        iold=i;
    }

    prof[nz+1]=prof[i-1];
    i=1;
    int j=1;
    do {
        do {
            i=i+1;
        } while (prof[i-1]<0.0);
        if(i-j!=1) {
            for(int k=j+1;k<=i-1;k++)
                prof[k-1]=prof[j-1]+((float)(k-j))*(prof[i-1]-prof[j-1])/((float)(i-j));
        }
        j=i;
    } while (j<nz+2);
}
//
//     Set up the profiles.
//
static
void profl( FILE* fs1,
        int nz, float dz, float eta, float omega, float rmax, float c0, float k0, float *rp, float cw[mz], float cb[mz], float rhob[mz],
        float attn[mz], float alpw[mz], float alpb[mz], float ksqw[mz][2], float ksqb[mz][2], float attw[mz]) {
    //
    zread(fs1, nz,dz,cw);
    zread(fs1, nz,dz,attw);
    zread(fs1, nz,dz,cb);
    zread(fs1, nz,dz,rhob);
    zread(fs1, nz,dz,attn);
    *rp=2.0*rmax;
    fscanf(fs1,"%f",rp);
    //
    for(int i=0; i< nz+2; i++) {
        double rtmp = (omega/cw[i]);
        double itmp = (omega/cw[i])*(eta*attw[i]);
        ksqw[i][0]=(rtmp*rtmp-itmp*itmp)-k0*k0;
        ksqw[i][1]=(rtmp*itmp+itmp*rtmp);
        rtmp = (omega/cb[i]);
        itmp = (omega/cb[i])*(eta*attn[i]);
        ksqb[i][0]=(rtmp*rtmp-itmp*itmp)-k0*k0;
        ksqb[i][1]=(rtmp*itmp+itmp*rtmp);
        alpw[i]=sqrt(cw[i]/c0);
        alpb[i]=sqrt(rhob[i]*cb[i]/c0);
    }
}

//
//     The tridiagonal matrices.
//
static
void matrc(int const nz, int const np, int const iz, float dz, float k0, float rhob[mz], float alpw[mz], float alpb[mz], float ksq[mz][2], float ksqw[mz][2], 
        float ksqb[mz][2], float f1[mz], float f2[mz], float f3[mz],
        float r1[mp][mz][2], float r2[mp][mz][2], float r3[mp][mz][2],
        float s1[mp][mz][2], float s2[mp][mz][2], float s3[mp][mz][2],
        float pd1[mp][2], float pd2[mz][2], int izsrf) {
    float d1[4] __attribute__((aligned(16))),d2[4] __attribute__((aligned(16))),d3[4] __attribute__((aligned(16))),rfact[2];
    //
    float a1=k0*k0/6.0;
    float a2=2.0*k0*k0/3.0;
    float a3=k0*k0/6.0;
    float cfact=0.5/(dz*dz);
    float dfact=1.0/12.0;
    //
    for(int i=0;i<iz; i++) {
        f1[i]=1.0/alpw[i];
        f2[i]=1.0;
        f3[i]=alpw[i];
        ksq[i][0]=ksqw[i][0];
        ksq[i][1]=ksqw[i][1];
    }
    //
    for(int i=iz,ii=0;i<nz+2; i++,ii++) {
        f1[i]=rhob[ii]/alpb[ii];
        f2[i]=1.0/rhob[ii];
        f3[i]=alpb[ii];
        ksq[i][0]=ksqb[ii][0];
        ksq[i][1]=ksqb[ii][1];
    }
    //
    for(int i=1; i< nz+1; i++) {
        //
        //     Discretization by Galerkin's method.
        //
        float c1=cfact*f1[i]*(f2[i-1]+f2[i])*f3[i-1];
        float c2=-cfact*f1[i]*(f2[i-1]+2.0*f2[i]+f2[i+1])*f3[i];
        float c3=cfact*f1[i]*(f2[i]+f2[i+1])*f3[i+1];
        d1[0]=c1+dfact*(ksq[i-1][0]+ksq[i][0]);
        d1[1]=dfact*(ksq[i-1][1]+ksq[i][1]);
        d2[0]=c2+dfact*(ksq[i-1][0]+6.0*ksq[i][0]+ksq[i+1][0]);
        d2[1]=dfact*(ksq[i-1][1]+6.0*ksq[i][1]+ksq[i+1][1]);
        d3[0]=c3+dfact*(ksq[i][0]+ksq[i+1][0]);
        d3[1]=dfact*(ksq[i][1]+ksq[i+1][1]);
        //
        for(int j=0;j<np;j++) {
            r1[j][i][0]=a1+pd2[j][0]*d1[0]-pd2[j][1]*d1[1];
            r1[j][i][1]= 0+pd2[j][1]*d1[0]+pd2[j][0]*d1[1];
            s1[j][i][0]=a1+pd1[j][0]*d1[0]-pd1[j][1]*d1[1];
            s1[j][i][1]= 0+pd1[j][1]*d1[0]+pd1[j][0]*d1[1];
            r2[j][i][0]=a2+pd2[j][0]*d2[0]-pd2[j][1]*d2[1];
            r2[j][i][1]= 0+pd2[j][1]*d2[0]+pd2[j][0]*d2[1];
            s2[j][i][0]=a2+pd1[j][0]*d2[0]-pd1[j][1]*d2[1];
            s2[j][i][1]= 0+pd1[j][1]*d2[0]+pd1[j][0]*d2[1];
            r3[j][i][0]=a3+pd2[j][0]*d3[0]-pd2[j][1]*d3[1];
            r3[j][i][1]= 0+pd2[j][1]*d3[0]+pd2[j][0]*d3[1];
            s3[j][i][0]=a3+pd1[j][0]*d3[0]-pd1[j][1]*d3[1];
            s3[j][i][1]= 0+pd1[j][1]*d3[0]+pd1[j][0]*d3[1];
        }
    }
    //
    //     The entries above the surface.
    //
    for(int j=0;j<np;j++) {
        for(int i=0;i<izsrf;i++) {
            r1[j][i][0]=0.0;
            r1[j][i][1]=0.0;
            r2[j][i][0]=1.0;
            r2[j][i][1]=0.0;
            r3[j][i][0]=0.0;
            r3[j][i][1]=0.0;
            s1[j][i][0]=0.0;
            s1[j][i][1]=0.0;
            s2[j][i][0]=0.0;
            s2[j][i][1]=0.0;
            s3[j][i][0]=0.0;
            s3[j][i][1]=0.0;
        }
    }
    //
    //     The matrix decomposition.
    //
    for(int j=0; j<np; j++) {
        for(int i=1;i<nz+1; i++) {
            double treal = r2[j][i][0]-(r1[j][i][0]*r3[j][i-1][0]-r1[j][i][1]*r3[j][i-1][1]);
            double timag = r2[j][i][1]-(r1[j][i][1]*r3[j][i-1][0]+r1[j][i][0]*r3[j][i-1][1]);
            double tnorm= treal*treal + timag*timag;
            rfact[0]= treal/tnorm;
            rfact[1]= -timag/tnorm;

            double tmp = r1[j][i][0];
            r1[j][i][0]=r1[j][i][0]*rfact[0]-r1[j][i][1]*rfact[1];
            r1[j][i][1]=r1[j][i][1]*rfact[0]+tmp*rfact[1];

            tmp = r3[j][i][0];
            r3[j][i][0]=r3[j][i][0]*rfact[0]-r3[j][i][1]*rfact[1];
            r3[j][i][1]=r3[j][i][1]*rfact[0]+tmp*rfact[1];

            tmp=s1[j][i][0];
            s1[j][i][0]=s1[j][i][0]*rfact[0]-s1[j][i][1]*rfact[1];
            s1[j][i][1]=s1[j][i][1]*rfact[0]+tmp*rfact[1];

            tmp=s2[j][i][0];
            s2[j][i][0]=s2[j][i][0]*rfact[0]-s2[j][i][1]*rfact[1];
            s2[j][i][1]=s2[j][i][1]*rfact[0]+tmp*rfact[1];

            tmp=s3[j][i][0];
            s3[j][i][0]=s3[j][i][0]*rfact[0]-s3[j][i][1]*rfact[1];
            s3[j][i][1]=s3[j][i][1]*rfact[0]+tmp*rfact[1];
        }
    }
}
//
//     Matrix updates.
//
static
void updat( FILE* fs1, int nz, int np, int *iz, int *ib,
        float dr, float dz, float eta, float omega, float rmax, float c0, float k0, 
        float r, float *rp, float rs,
        float rb[mr], float zb[mr] , float cw[mz], float cb[mz], float rhob[mz],
        float attn[mz], float alpw[mz], float alpb[mz],
        fcomplex ksq[mz], fcomplex ksqw[mz], fcomplex ksqb[mz],
        float f1[mz], float f2[mz], float f3[mz],
        fcomplex r1[mp][mz], fcomplex r2[mp][mz], fcomplex r3[mp][mz],
        float s1[mp][mz][2], float s2[mp][mz][2], float s3[mp][mz][2],
        fcomplex pd1[mp], fcomplex pd2[mp],
        float rsrf[mr], float zsrf[mr], int *izsrf, int *isrf, float attw[mz]) {
    //
    //     Varying bathymetry.
    //
    while(r>=rb[*ib]) (*ib)++;
    while(r>=rsrf[*isrf]) (*isrf)++;
    //
    int jzsrf=*izsrf;
    float z=zsrf[*isrf-1]+(r+0.5*dr-rsrf[*isrf-1])*
        (zsrf[*isrf]-zsrf[*isrf-1])/(rsrf[*isrf]-rsrf[*isrf-1]);
    *izsrf=z/dz;
    //
    int jz=*iz;
    z=zb[*ib-1]+(r+0.5*dr-rb[*ib-1])*(zb[*ib]-zb[*ib-1])/(rb[*ib]-rb[*ib-1]);
    *iz=1.0+z/dz;
    *iz=max(2,*iz);
    *iz=min(nz,*iz);
    if((*iz!=jz) || (*izsrf != jzsrf))
        matrc(nz, np, *iz, dz, k0, rhob, alpw, alpb, (float (*)[2])ksq, (float (*)[2])ksqw, (float (*)[2])ksqb, 
                f1, f2, f3, (float (*)[mz][2])r1, (float (*)[mz][2])r2, (float (*)[mz][2])r3, s1, s2, s3, (float (*)[2])pd1, (float (*)[2])pd2, *izsrf);
    //
    //     Varying profiles.
    //
    if(r>=*rp){
        profl(fs1, nz, dz, eta, omega, rmax, c0, k0, rp, cw, cb, rhob, attn, 
                alpw, alpb, (float (*)[2])ksqw, (float (*)[2])ksqb, attw);
        matrc(nz, np, *iz, dz, k0, rhob, alpw, alpb, (float (*)[2])ksq, (float (*)[2])ksqw, (float (*)[2])ksqb, 
                f1, f2, f3, (float (*)[mz][2])r1, (float (*)[mz][2])r2, (float (*)[mz][2])r3, s1, s2, s3, (float (*)[2])pd1, (float (*)[2])pd2, *izsrf);
    }
    //
    //     Turn off the stability constraints.
    //
    if(r>=rs) {
        int ns=0;
        epade(np, ns, 1, k0, dr, (float (*)[2])pd1, (float (*)[2])pd2);
        matrc(nz, np, *iz, dz, k0, rhob, alpw, alpb, (float (*)[2])ksq, (float (*)[2])ksqw, (float (*)[2])ksqb, 
                f1, f2, f3, (float (*)[mz][2])r1, (float (*)[mz][2])r2, (float (*)[mz][2])r3, s1, s2, s3, (float (*)[2])pd1, (float (*)[2])pd2, *izsrf);
    }
    //
}

//
//     Output transmission loss.
//
static
void  outpt( FILE* fs2, FILE* fs3, int *mdr, int ndr, int ndz, int nzplt, int lz, int ir, float dir, float eps, float r,
        float f3[mz], float u[mz][2], float tlg[mz]) {
    fcomplex ur;
    //
    ur=(1.0f-dir)*(f3[ir-1]*(u[ir-1][0]+I*u[ir-1][1]))+dir*f3[ir]*(u[ir][0]+I*u[ir][1]);
    float tl=-20.0*log10(cabs(ur)+eps)+10.0*log10(r+eps);
    fprintf(fs2,"%f %f\n",r,tl);
    //
    *mdr=*mdr+1;
    if(*mdr==ndr) {
        *mdr=0;
        //
        int j=0;
        for(int i=ndz-1; i< nzplt; i+=ndz) {
            ur=(u[i][0]+I*u[i][1])*f3[i];
            tlg[j]=-20*log10((cabs(ur)+eps)/sqrt(r+eps));
            j=j+1;
            //
        }
       int n = lz * sizeof(tlg[0]);
       fwrite((char*)&n, sizeof(n), 1, fs3); //FORTRAN record header
       fwrite((char*)tlg,sizeof(tlg[0]),lz,fs3);
       fwrite((char*)&n, sizeof(n), 1, fs3); //FORTRAN record footer
    }
    //
}

//
//     The tridiagonal solver.
//
static
void solve(int nz, int const np, float u[mz][2], 
        fcomplex r1[mp][mz], fcomplex r3[mp][mz],
        float s1[mp][mz][2], float s2[mp][mz][2], float s3[mp][mz][2]) {
    float v[nz][2];
    for(int j=0; j<np; j++) {
        float *R1 = (float*) r1[j];
        float *R3 = (float*) r3[j];
        for(int i=1; i< nz+1; i++) {
            v[i][0] = s1[j][i][0]*u[i-1][0]-s1[j][i][1]*u[i-1][1] + s2[j][i][0]*u[i][0] - s2[j][i][1]*u[i][1] + s3[j][i][0]*u[i+1][0] - s3[j][i][1]*u[i+1][1];
            v[i][1] = s1[j][i][0]*u[i-1][1]+s1[j][i][1]*u[i-1][0] + s2[j][i][0]*u[i][1] + s2[j][i][1]*u[i][0] + s3[j][i][0]*u[i+1][1] + s3[j][i][1]*u[i+1][0];
        }
        //
        for(int i=2;i<nz+1; i++) {
            v[i][0]-=R1[2*i]*v[i-1][0] - R1[2*i+1]*v[i-1][1];
            v[i][1]-=R1[2*i]*v[i-1][1] + R1[2*i+1]*v[i-1][0];
        }
        //
        for(int i=nz; i>=1; i--) {
            u[i][0] = v[i][0] - (R3[2*i] * u[i+1][0] - R3[2*i+1] * u[i+1][1]);
            u[i][1] = v[i][1] - (R3[2*i] * u[i+1][1] + R3[2*i+1] * u[i+1][0]);
        }
        //
    }
    //
}

//
//     The self-starter.
//
static
void selfs(int nz, int np, int ns, int iz, float zs, float dr, float dz, float k0, float rhob[mz], float alpw[mz], 
        float alpb[mz], fcomplex ksq[mz], fcomplex ksqw[mz], fcomplex ksqb[mz],
        float f1[mz], float f2[mz], float f3[mz], float u[mz][2], 
        fcomplex r1[mp][mz], fcomplex r2[mp][mz], fcomplex r3[mp][mz],
        float s1[mp][mz][2], float s2[mp][mz][2], float s3[mp][mz][2],
        fcomplex pd1[mp], fcomplex pd2[mp], 
        int izsrf) {
    //
    //     Conditions for the delta function.
    //
    float si=1.0+zs/dz;
    int is=(int)si;
    float dis=si-(float)is;
    u[is-1][0]=(1.0-dis)*sqrt(2.0*M_PI/k0)/(dz*alpw[is-1]);
    u[is-1][1]=0.f;
    u[is][0]=dis*sqrt(2.0*M_PI/k0)/(dz*alpw[is-1]);
    u[is][1]=0.f;
    //
    //     Divide the delta function by [1-X]**2 to get a smooth rhs.
    //
    pd1[0]=0.0;
    pd2[0]=-1.0;
    matrc(nz, 1, iz, dz, k0, rhob, alpw, alpb, (float (*)[2])ksq, (float (*)[2])ksqw, (float (*)[2])ksqb, 
            f1, f2, f3, (float (*)[mz][2])r1, (float (*)[mz][2])r2, (float (*)[mz][2])r3, s1, s2, s3, (float (*)[2])pd1, (float (*)[2])pd2, izsrf);
    solve( nz, 1, u, r1, r3, s1, s2, s3);
    solve( nz, 1, u, r1, r3, s1, s2, s3);
    //
    //     Apply the operator [1-X]**2*[1+X]**[-1/4]*exp[ci*k0*r*sqrt[1+X]].
    //
    epade(np,ns,2,k0,dr,(float (*)[2])pd1,(float (*)[2])pd2);
    matrc(nz, np, iz, dz, k0, rhob, alpw, alpb, (float (*)[2])ksq, (float (*)[2])ksqw, (float (*)[2])ksqb, 
            f1, f2, f3, (float (*)[mz][2])r1, (float (*)[mz][2])r2, (float (*)[mz][2])r3, s1, s2, s3, (float (*)[2])pd1, (float (*)[2])pd2, izsrf);
    solve( nz, np, u, r1, r3, s1, s2, s3);
    //
}


//
//     Initialize the parameters, acoustic field, and matrices.
//
static
void setup( FILE* fs1, FILE* fs2, FILE* fs3,
        int *nz, int *np, int *ns, int *mdr, int *ndr, int *ndz, int *iz,
        int *nzplt, int *lz, int *ib, int *ir,
        float *dir, float *dr, float *dz, float *eta, float *eps, float *omega, float *rmax,
        float *c0, float *k0, fcomplex *ci, float *r, float *rp, float *rs, float rb[mr], float zb[mr], float cw[mz], float cb[mz], float rhob[mz],
        float attn[mz], float alpw[mz], float alpb[mz], fcomplex ksq[mz], fcomplex ksqw[mz], fcomplex ksqb[mz],
        float f1[mz], float f2[mz], float f3[mz],
        float u[mz][2], 
        fcomplex r1[mp][mz], fcomplex r2[mp][mz], fcomplex r3[mp][mz],
        float s1[mp][mz][2], float s2[mp][mz][2], float s3[mp][mz][2],
        fcomplex pd1[mp], fcomplex pd2[mp], float tlg[mz], float rsrf[mr], float zsrf[mr], int *izsrf, int *isrf, float attw[mz]) {
    float freq, zs,zr,zmax,zmplt;

    char tmp[BUFSIZE];
    //
    fgets(tmp,BUFSIZE,fs1);
    fscanf(fs1,"%f %f %f",&freq,&zs,&zr);
    fgets(tmp,BUFSIZE,fs1);
    fscanf(fs1,"%f %f %d",rmax,dr,ndr);
    fgets(tmp,BUFSIZE,fs1);
    fscanf(fs1,"%f %f %d %f",&zmax,dz,ndz,&zmplt);
    fgets(tmp,BUFSIZE,fs1);
    fscanf(fs1,"%f %d %d %f",c0,np,ns,rs);
    fgets(tmp,BUFSIZE,fs1);
    //
    int i=0;
    while(1) {
        if (fscanf(fs1,"%f %f",&rsrf[i],&zsrf[i])!=2)
    	    longjmp(exception_env,PARSE_ERROR);
        fgets(tmp,BUFSIZE,fs1);
        if( rsrf[i] < 0.0) break;
        i=i+1;
    }

    rsrf[i]=2.0*(*rmax);
    zsrf[i]=zsrf[i-1];
    //
    i=0;
    while(1) {
        if (fscanf(fs1,"%f %f",&rb[i],&zb[i])!=2)
            longjmp(exception_env,PARSE_ERROR);
        fgets(tmp,BUFSIZE,fs1);
        if( rb[i] < 0.0) break;
        i=i+1;
    }

    rb[i]=2.0*(*rmax);
    zb[i]=zb[i-1];
    //
    *ci=I;
    *eta=1.0/(40.0*M_PI*log10(exp(1.0)));
    *eps=1.0e-20;
    *ib=1;
    *isrf=1;
    *mdr=0;
    *r=*dr;
    *omega=2.0*M_PI*freq;
    float ri=1.0+zr/(*dz);
    *ir=(int)ri;
    *dir=ri-(float)(*ir);
    *k0=*omega/(*c0);
    *nz=zmax/(*dz)-0.5;
    *nzplt=zmplt/(*dz)-0.5;
    //
    float z=zsrf[0];
    *izsrf=1.0+z/(*dz);
    //
    z=zb[0];
    *iz=1.0+z/(*dz);
    *iz=max(2,*iz);
    *iz=min(*nz,*iz);
    if(*rs < *dr){*rs=2.0*(*rmax);}
    //
    if(*nz+2>mz) {
        //std::cerr <<  "Need to increase parameter mz to " << nz+2 << std::endl;
        longjmp(exception_env,INC_MZ);
    }
    if(*np>mp) {
        //std::cerr <<  "Need to increase parameter mp to " << np << std::endl;
        longjmp(exception_env,INC_MP);
    }
    if(i>mr) {
        //std::cerr <<  "Need to increase parameter mr to " << i << std::endl;
        longjmp(exception_env,INC_MR);
    }
    //
    for(int j=0;j<mp; j++) {
        r3[j][0]=0.0;
        r1[j][*nz+1]=0.0;
    }
    for(int i=0; i< *nz+2; i++) {
        u[i][0]=0.0;
        u[i][1]=0.0;
    }

    *lz=0;
    for(int i=(*ndz); i<(*nzplt); i+=(*ndz)) {
        *lz=*lz+1;
    }
    int lz_size = sizeof(*lz);
    fwrite((char*)&lz_size, sizeof(lz_size), 1, fs3); //FORTRAN RECORD HEADER
    fwrite((char*)lz,sizeof(*lz),1,fs3);
    fwrite((char*)&lz_size, sizeof(lz_size), 1, fs3); //FORTRAN RECORD FOOTER
    //
    //     The initial profiles and starting field.
    //
    profl(fs1, *nz, *dz, *eta, *omega, *rmax, *c0, *k0, rp, cw, cb, rhob, attn, 
            alpw, alpb, (float (*)[2])ksqw, (float (*)[2])ksqb, attw);
    selfs(*nz, *np, *ns, *iz, zs, *dr, *dz, *k0, rhob, alpw, alpb, ksq, 
            ksqw, ksqb, f1, f2, f3, u, r1, r2, r3, s1, s2, s3, pd1, pd2, *izsrf);
    outpt(fs2,  fs3,  mdr, *ndr, *ndz, *nzplt, *lz, *ir, *dir, *eps, *r, f3, u, tlg);
    //
    //     The propagation matrices.
    //
    epade(*np, *ns, 1, *k0, *dr, (float (*)[2])pd1, (float (*)[2])pd2);
    matrc(*nz, *np, *iz, *dz, *k0, rhob, alpw, alpb, (float (*)[2])ksq, (float (*)[2])ksqw, (float (*)[2])ksqb, 
            f1, f2, f3, (float (*)[mz][2])r1, (float (*)[mz][2])r2, (float (*)[mz][2])r3, s1, s2, s3, (float (*)[2])pd1, (float (*)[2])pd2, *izsrf);
    //
}

int ramsurf(char const* input, char const* resultat, char const* output) {
    FILE* fs1 = fopen(input,"r");
    FILE* fs2 = fopen(output,"w+");
    FILE* fs3 = fopen(resultat,"w+");
    fcomplex ci, (*ksq)[mz], (*ksqb)[mz], (*r1)[mp][mz], (*r2)[mp][mz], 
            (*r3)[mp][mz],  (*pd1)[mp], (*pd2)[mp], 
            (*ksqw)[mz];
    float (*u)[mz][2], (*s1)[mp][mz][2], (*s2)[mp][mz][2], (*s3)[mp][mz][2];
    float k0, (*rb)[mr], (*zb)[mr], (*cw)[mz], (*cb)[mz], (*rhob)[mz], (*attn)[mz], (*alpw)[mz], 
          (*alpb)[mz], (*f1)[mz], (*f2)[mz], (*f3)[mz], (*tlg)[mz], (*rsrf)[mr], 
          (*zsrf)[mr], (*attw)[mz];
    int errorCode;

    // allocation step 
    
    ksq = (fcomplex (*)[mz]) malloc(sizeof(fcomplex)*mz);
    ksqb = (fcomplex (*)[mz]) malloc(sizeof(fcomplex)*mz);
    ksqw = (fcomplex (*)[mz]) malloc(sizeof(fcomplex)*mz);
    r1 = (fcomplex (*)[mp][mz]) malloc(sizeof(fcomplex)*mz*mp);
    r2 = (fcomplex (*)[mp][mz]) malloc(sizeof(fcomplex)*mz*mp);
    r3 = (fcomplex (*)[mp][mz]) malloc(sizeof(fcomplex)*mz*mp);
    pd1 = (fcomplex (*)[mp]) malloc(sizeof(fcomplex)*mp);
    pd2 = (fcomplex (*)[mp]) malloc(sizeof(fcomplex)*mp);
    rb = (float (*)[mr]) malloc(sizeof(float)*mr);
    zb = (float (*)[mr]) malloc(sizeof(float)*mr);
    rsrf = (float (*)[mr]) malloc(sizeof(float)*mr);
    zsrf = (float (*)[mr]) malloc(sizeof(float)*mr);
    cw = (float (*)[mz]) malloc(sizeof(float)*mz);
    cb = (float (*)[mz]) malloc(sizeof(float)*mz);
    rhob = (float (*)[mz]) malloc(sizeof(float)*mz);
    attn = (float (*)[mz]) malloc(sizeof(float)*mz);
    alpw = (float (*)[mz]) malloc(sizeof(float)*mz);
    alpb = (float (*)[mz]) malloc(sizeof(float)*mz);
    f1 = (float (*)[mz]) malloc(sizeof(float)*mz);
    f2 = (float (*)[mz]) malloc(sizeof(float)*mz);
    f3 = (float (*)[mz]) malloc(sizeof(float)*mz);
    tlg = (float (*)[mz]) malloc(sizeof(float)*mz);
    attw = (float (*)[mz]) malloc(sizeof(float)*mz);
    u = (float (*)[mz][2]) malloc(sizeof(float)*mz*2);
    s1 = (float (*)[mp][mz][2]) malloc(sizeof(float)*mp*mz*2);
    s2 = (float (*)[mp][mz][2]) malloc(sizeof(float)*mp*mz*2);
    s3 = (float (*)[mp][mz][2]) malloc(sizeof(float)*mp*mz*2);

    int nz,np,ns,mdr,ndr,ndz,iz,nzplt,lz,ib,ir,izsrf,isrf;
    float eta, eps, omega, rmax, r, rp, rs, dr, dz, c0, dir;
    if(!(errorCode=setjmp(exception_env)))
    {
        setup(fs1, fs2, fs3, &nz, &np, &ns, &mdr, &ndr, &ndz, &iz,
                &nzplt, &lz, &ib, &ir,
                &dir, &dr, &dz, &eta, &eps, &omega, &rmax,
                &c0, &k0, &ci, &r, &rp, &rs, *rb, *zb, *cw, *cb, *rhob, 
                *attn, *alpw, *alpb, *ksq, *ksqw, *ksqb, 
                *f1, *f2, *f3, 
                *u, 
                *r1, *r2, *r3, *s1, *s2, *s3, 
                *pd1, *pd2, *tlg, *rsrf, *zsrf, &izsrf, &isrf, *attw);
        //
        //     March the acoustic field out in range.
        //
        while (r < rmax) {
            updat(fs1, nz, np, &iz, &ib, dr, dz, eta, omega, rmax, c0, k0, r, 
                    &rp, rs, *rb, *zb, *cw, *cb, *rhob, *attn, *alpw, *alpb, *ksq, *ksqw, *ksqb, *f1, *f2, *f3, 
                    *r1, *r2, *r3, *s1, *s2, *s3, *pd1, *pd2, *rsrf, *zsrf, &izsrf, &isrf, *attw);
            solve(nz, np, *u, *r1, *r3, *s1, *s2, *s3);
            r=r+dr;
            outpt(fs2,  fs3,  &mdr, ndr, ndz, nzplt, lz, ir, dir, eps, r, *f3, *u, *tlg);
        }
    }

    fclose(fs2);
    fclose(fs3);
    // deallocation step 
    free(ksq); 
    free(ksqb);
    free(ksqw);
    free(r1);
    free(r2);
    free(r3);
    free(pd1);
    free(pd2);
    free(rb);
    free(zb);
    free(rsrf);
    free(zsrf);
    free(cw);
    free(cb);
    free(rhob);
    free(attn);
    free(alpw);
    free(alpb);
    free(f1);
    free(f2);
    free(f3);
    free(tlg);
    free(attw);
    free(u) ;
    free(s1);
    free(s2);
    free(s3);

    return errorCode;
}

//
//     This version of ram handles variable surface height, including
//     rough surfaces and propagation in beach environments [J. Acoust.
//     Soc. Am. 97, 2767-2770 (1995)]. The input file is similar to the 
//     input file for ram but contains a block of data for the location 
//     of the surface. The computational grid extends from z=0 to z=zmax. 
//     The pressure release surface is located at z=zsrf(r), which is 
//     linearly interpolated between input points just like the bathymetry. 
//     The inputs zsrf must be greater than or equal to zero. As in ramgeo, 
//     the layering in the sediment parallels the bathymetry. 
//
//     ******************************************************************
//     ***** Range-dependent Acoustic Model, Version 1.5b, 19-Oct-00 ****
//     ******************************************************************
//
#ifndef RAMSURF_NO_MAIN
int main(int argc, char *argv[]) {
    ramsurf("ram.in", "tl.grid", "tl.line");
    return 0;
}
#endif

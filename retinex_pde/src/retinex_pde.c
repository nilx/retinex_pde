
/* mwcommand
 name = {retinex_pde};
 author = {"Catalina-Ana-Jean-Michel"};
 version = {"1.1"};
 function = {"Implementation of Land Retinex Theory"};
 usage = {

	   't':[t=0.]-> t     "threshold of the directional gradient (defecte 0.)",
	   in->in            "input color image",
	   out1<-out1         "white balance image output",
           out2<-out2          "retinex processed image"
};
*/

#include <stdio.h>
#include "mw.h"
#include <math.h>

#define _(a,i,j)  ((a)->gray[(j)*(a)->ncol+(i)])

#define r(a,i,j)  ((a)->red[(j)*(a)->ncol+(i)])
#define g(a,i,j)  ((a)->green[(j)*(a)->ncol+(i)])
#define b(a,i,j)  ((a)->blue[(j)*(a)->ncol+(i)])

extern void fft2d();
extern void fsym2();
extern void fshrink2();
extern void fextract();

int nx,ny,dim;
int Nx,Ny,DIM;

float *terme_indep(in,t)
Fimage in;
float t;
{
     int i,j,ii;
      double E,sum;
      float *F;



   if((F=(float*)calloc(sizeof(float),dim))==NULL)
      mwerror(FATAL,1, "no memoria\n");



      for(i=0;i<nx;i++){
        for(j=0;j<ny;j++){
	  sum=0.;

	  if(i ==0 ) ii=0; else ii=i-1;
          E=_(in,i,j)-_(in,ii,j);
	  if(fabs(E)> t) sum=sum+ E;

	  if(i==nx-1) ii=nx-1; else ii=i+1;
          E= _(in,i,j)-_(in,ii,j);
	  if(fabs(E)> t) sum=sum+ E;
	    
          if(j==0) ii=0; else ii=j-1;
           E= _(in,i,j)-_(in,i,ii);
	  if(fabs(E)> t)  sum=sum+ E;
	   
          if(j== ny-1) ii=ny-1; else ii=j+1;
	   E=_(in,i,j)-_(in,i,ii);
	  if(fabs(E)> t) sum=sum+E;
	   
	  F[i+j*nx]=(float) sum;

        }
      }
      return(F);



}



void min_max(p,min,max)
float *p,*min,*max;
{
    int l;
    float card,np;

    card=0.03*dim;

    *min=p[0]; *max=p[0];
    for(l=1;l<dim;l++){
      if(p[l]<*min) *min=p[l];
      if(p[l]>*max) *max=p[l];
     }

      np=0;

     while(np < card){
       *min=*min+0.5;
       for(l=0;l<dim;l++){
          if(p[l] <= *min) np=np+1;
       }
     }


     np=0;

    while(np<card){
     *max=*max-0.5;
     for(l=0;l<dim;l++){
          if(p[l] >= *max) np=np+1;
       }
     }


}
float *normalitzat(p)
float *p;
{
     int i;
     float max,min;
     float *F;

     if((F=(float*)calloc(sizeof(float),dim))==NULL)
      mwerror(FATAL,1, "no memoria\n");

    min_max(p,&min,&max);

     for(i=0;i<dim;i++){
      if(p[i]< min) F[i]=0.;
       else if(p[i]>max) F[i]=255.;
       else F[i]=255.*(p[i]-min)/(max-min);
     

     }


      return(F);
}




retinex_pde(in,out1,out2,t)
Cfimage in;
Cfimage out1, out2;
float *t;
{

    int l,x,y;
    double theta1,theta2,den;
    Fimage ci=NULL,ci1=NULL,ci2=NULL;
    char i;
    float b,z;


   nx=in->ncol; ny=in->nrow;
   dim=nx*ny;
   printf("nx=%d ny=%d \n", nx,ny);
  
   out1=mw_change_cfimage(out1,ny,nx);
  if (out1==NULL) mwerror(FATAL,1,"Not enough memory \n");

    out2=mw_change_cfimage(out2,ny,nx);
  if (out2==NULL) mwerror(FATAL,1,"Not enough memory \n");

   ci=mw_change_fimage(ci,ny,nx);
  if (ci==NULL) mwerror(FATAL,1,"Not enough memory \n");
   
   ci1=mw_change_fimage(ci1,2*ny,2*nx);
  if (ci1==NULL) mwerror(FATAL,1,"Not enough memory \n");
  ci2=mw_change_fimage(ci2,2*ny,2*nx);
  if (ci2==NULL) mwerror(FATAL,1,"Not enough memory \n");

/* The white balance of the input image*/
    out1->red=normalitzat(in->red);
    out1->green=normalitzat(in->green);
    out1->blue=normalitzat(in->blue);


/*Retinex Implementation on the output of the white balance*/


  printf("red\n");

   
     for(l=0;l<dim;l++) ci->gray[l]=out1->red[l];
    
     ci->gray=terme_indep(ci,*t); /* f(I)  of the red channel*/

     fsym2(ci,ci1,NULL);  /* simmetrization of the image*/

     mw_clear_fimage(ci2,0.0);  /* imaginary part of the TF */

     fft2d(ci1,ci2,ci1,ci2,NULL);
     for(x=0;x< 2*nx;x++){
       for(y=0;y< 2*ny;y++){
	   theta1=M_PI*x/nx; theta2=M_PI*y/ny;
	   den=2.*(2.-cos(theta1)-cos(theta2)+2./(dim-1));
	   _(ci1,x,y)=_(ci1,x,y)/den;
	   _(ci2,x,y)=_(ci2,x,y)/den;
        }
     }

     i=1;

     fft2d(ci1,ci2,ci1,ci2,&i);
     
     fsym2(ci1,ci,&i);

     out2->red=normalitzat(ci->gray);
  



   printf("green\n");

   for(l=0;l<dim;l++) ci->gray[l]=out1->green[l];
    
   ci->gray=terme_indep(ci,*t);

   fsym2(ci,ci1,NULL);
   
   mw_clear_fimage(ci2,0.0);

   fft2d(ci1,ci2,ci1,ci2,NULL);

   for(x=0;x< 2*nx;x++){
     for(y=0;y< 2*ny;y++){
	   theta1=M_PI*x/nx; theta2=M_PI*y/ny;
	   den=2.*(2.-cos(theta1)-cos(theta2)+2./(dim-1));
	   _(ci1,x,y)=_(ci1,x,y)/den;
	   _(ci2,x,y)=_(ci2,x,y)/den;
	   
        }
     }
     i=1;

     fft2d(ci1,ci2,ci1,ci2,&i);
  
     fsym2(ci1,ci,&i);

     out2->green=normalitzat(ci->gray);

 
    printf("blue\n");
 
   for(l=0;l<dim;l++)  ci->gray[l]=out1->blue[l];

   ci->gray=terme_indep(ci,*t);

   fsym2(ci,ci1,NULL);

   mw_clear_fimage(ci2,0.0);

   fft2d(ci1,ci2,ci1,ci2,NULL);

   for(x=0;x< 2*nx;x++){
     for(y=0;y< 2*ny;y++){
	   theta1=M_PI*x/nx; theta2=M_PI*y/ny;
	   den=2.*(2.-cos(theta1)-cos(theta2)+2./(dim-1));
	   _(ci1,x,y)=_(ci1,x,y)/den;
	   _(ci2,x,y)=_(ci2,x,y)/den;
	   
        }
     }
     i=1;
   
     fft2d(ci1,ci2,ci1,ci2,&i);
   
     fsym2(ci1,ci,&i);
    
     out2->blue=normalitzat(ci->gray);
    
 }



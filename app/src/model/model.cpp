// Copyright (C) 2015 Foam Kernow
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.

#include <iostream>
#include "model.h"
#include "range_spedup_final.h"
#include "jellyfish/core/types.h"

using namespace red_king;
using namespace std;

/******************************************
 * Parameters that could be changed via GUI
 ******************************************/

/* Function parameters */
#define BETMIN 0.491
#define BEMAXTIME 17.117
#define AMIN 1.782
#define AMAX 5.454
#define A_P 2.615

#define BETA_P -0.434

#define WHO 0.5	/* Controls relative mutation rates */
#define EPSILON 0 /* Extinction tolerance */

/******************************************
 * Fixed parameters for solver
 ******************************************/
#define UMIN 0.0 /* Minimum host trait */
#define UMAX 10.0 /* Maximum host trait */
#define VMIN 0.0 /* Minimum parasite trait */
#define VMAX 10.0 /* Maximum parasite trait */

void clear(rk_real *ptr) {
  for (int i=0; i<N; i++) {
    ptr[i]=0;
  }
}

void mclear(rk_real **ptr) {
  for (int i=0; i<N; i++) {
    for (int j=0; j<N; j++) {
      ptr[i][j]=0;
    }
  }
}

void cleari(int *ptr) {
  for (int i=0; i<N; i++) {
    ptr[i]=0;
  }
}

model::model() {
  m_range = new range();

  u = new rk_real[N];
  v = new rk_real[N];
  a = new rk_real[N];
  beta = new rk_real[N];
  E = new rk_real*[N];
  for (u32 i=0; i<N; i++) E[i]=new rk_real[N];

  x0 = new rk_real[N];
  y = new rk_real[N];
  y0 = new rk_real*[N];
  for (u32 i=0; i<N; i++) y0[i]=new rk_real[N];
  host_ind=new int[N];
  par_ind=new int[N];

  init();
}

void model::init() {

  clear(u);
  clear(v);
  clear(a);
  clear(beta);
  mclear(E);
  clear(x0);
  clear(y);
  mclear(y0);
  cleari(host_ind);
  cleari(par_ind);

  /* Initialise population numbers */
  for (u32 i=0; i<N; i++)	{
    x0[i]=0.0;
    y[i]=0.0;
    for (int j=0; j<N; j++) {
      y0[i][j]=0.0;
    }
  }

  for (int i=0; i<5; i++) {
    int hstart = rand()%N;
    int pstart = rand()%N;
    x0[hstart]=1.0;
    y0[hstart][pstart]=1.0;
    y[pstart-1]=y0[hstart-1][pstart-1];
  }

  init_trait_values();
  init_cost_functions(AMIN, AMAX,
                      UMIN, UMAX,
                      A_P,
                      BETMIN,  BEMAXTIME,
                      VMIN,  VMAX,
                      BETA_P);
  init_matrix();
}

void model::init_trait_values() {
  for (int i=0; i<N; i++) {
    u[i]=UMIN+(UMAX-UMIN)*i/(N-1); /* Host */
    v[i]=VMIN+(VMAX-VMIN)*i/(N-1); /* Parasite */
  }
}

void model::init_cost_functions(rk_real amin, rk_real amax,
                                rk_real umin, rk_real umax,
                                rk_real a_p,
                                rk_real betmin, rk_real bemaxtime,
                                rk_real vmin, rk_real vmax,
                                rk_real beta_p) {
  /**********************************************************************
   * This section is where the trade-offs and host-parasite interactions
   * are defined (i.e. where the user would make changes via a GUI)
   *********************************************************************/
  /* Define cost functions (trade-offs) */
  for (int i=0; i<N; i++) {
    a[i]=amax-(amax-amin)*(1-(u[i]-umax)/(umin-umax))/(1+a_p*(u[i]-umax)/(umin-umax)); /* Host trade-off */
    beta[i]=bemaxtime-(bemaxtime-betmin)*(1-(v[i]-vmax)/(vmin-vmax))/(1+beta_p*(v[i]-vmax)/(vmin-vmax)); /* Parasite trade-off */
  }
}

void model::init_matrix() {
  /* Define host-parasite interaction matrix */
  for (int i=0; i<N; i++){
    for (int j=0; j<N; j++){
      E[i][j] = beta[j]*(1-1/(1+exp(-2*(u[i]-v[j]))));
    }
  }
}


void model::step() {
  int nh = 0;
  int np = 0;
  check_phenotypes(nh,np);

  /* Call ODE solver */
  m_range->my_rungkut(x0,y0,u,v,E,a,nh,np,host_ind,par_ind);

  // Check for extinct phenotypes
  for (int i=0; i<N; i++) {
    y[i]=0;
    if (x0[i]<EPSILON) x0[i]=0;
    for (int j=0; j<N; j++) {
      /*if (y0[j][i]<EPSILON) {
        y0[j][i]=0;
      }
      else*/ {
        // Work out parasite phenotype densities
        y[i]+=y0[j][i];
      }
    }
  }

  mutate();
}



void model::check_phenotypes(int &nh, int& np) {
  /* Find which phenotypes are present */
  for (int i=0; i<N; i++)	{
    //cerr<<x0[i]<<" "<<y[i]<<endl;
    if(x0[i]>0){
      host_ind[nh]=i;
      nh++;
    }
    if(y[i]>0){
      par_ind[np]=i;
      np++;
    };
  }

  std::cout <<  "Hosts:" << nh << ". Parasites:" << np << "\n";

  /* Check if all hosts or parasites have been driven extinct */
  if(nh==0) {
    for (int i=0; i<N; i++) {
      x0[i]=0;
      y[i]=0;
    }
    printf("Breaking - hosts driven extinct\n");
    return;
  }
  if(np==0){
    for (int i=0; i<N; i++) {
      y[i]=0;
    }
    printf("Breaking - parasites driven extinct\n");
    return;
  }
}

void model::mutate() {

    /* Mutation routine */
    rk_real rtype=rk_real(rand())/RAND_MAX;
    int pop_choice=1;
    if (rtype<WHO) pop_choice=0;

    rk_real r1=rk_real(rand())/RAND_MAX;
    rk_real xtotal = 0.0;
    rk_real xcum = 0.0;
    for (int i=0; i<N; i++) {
        if (pop_choice==0) xtotal+=x0[i];
        if (pop_choice==1) xtotal+=y[i];
    }

    int mutator = 0;

    for (int i=0; i<N; i++) {
        if (pop_choice==0) xcum+=x0[i];
        if (pop_choice==1) xcum+=y[i];
        if (r1 < xcum/xtotal) {	/* Find which strain will mutate */
            mutator=i;
            break;
        }
    }

    rk_real r2=rk_real(rand())/RAND_MAX;
    if (r2<0.5 && mutator>0){	/* Mutate up or down? */
        if (pop_choice==0) {
            x0[mutator-1]+=x0[mutator]/10.0;
            for (int j=0; j<N; j++) {
                y0[mutator-1][j]+=y0[mutator][j]/10.0;
            }
        }
        else for (int j=0; j<N; j++) y0[j][mutator-1]+=y0[j][mutator]/10.0;
    }
    else if (r2>0.5 && mutator<N-1){
        if (pop_choice==0) {
            x0[mutator+1]+=x0[mutator]/10.0;
            for (int j=0; j<N; j++) {
                y0[mutator+1][j]+=y0[mutator][j]/10.0;
            }
        }
        else for (int j=0; j<N; j++) y0[j][mutator+1]+=y0[j][mutator]/10.0;
    }
}

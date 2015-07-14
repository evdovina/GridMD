/****************************************************************************
 *
 *   Copyright (c), Ilya Valuev, Igor Morozov 2005-2010        All Rights Reserved.
 *
 *   Author	: Ilya Valuev, Igor Morozov, JIHT RAS, Moscow, Russia
 *
 *   Project	: GridMD
 *
 *   This source code is Free Software and distributed under the terms of GridMD license
 *   (gridmd\doc\gridmd_license.txt)
 *
 *   The example "fork_skeleton" performs calculation of first N terms of the series
 *                    x^2   x^3         x^N
 *   exp(x) = 1 + x + --- + --- + ... + --- + ...
 *                     2     6           N!
 *   Each term is calculated as a separate job.
 *   The example shows how to use gmFork template and split-merge skeleton.
 *
 *****************************************************************************/

#include "gridmd.h"
#include "gmfork.h"
#include <cmath>


using namespace gridmd;


int main(int argc,char* argv[]){

  // wxWindows initialization
  if( !wxInitialize() ) {
    puts("Failed to initialize the wxWidgets library.");
    return -1;
  }

  // The following command enables output of information messages (vblALLMESS),
  // warnings and errors (vblALLBAD). Remove 'vblALLMESS' to get rid of 
  // many information messages during execurion.
  // The program is interrupted on errors (vblERR).
  message_logger::global().set_levels(vblALLBAD | vblALLMESS/*, vblERR*/);
  
  // All distributed code should be put into gridmd_main() function
  int res= gridmd_main(argc, argv);

  wxUninitialize();
  return res;
}


// Note that this function may be called recursively.
// Use global variables with care!
int gridmd_main(int argc,char* argv[]){

  // GridMD initialization and processing of the command line arguments
  gmExperiment.init(argc,argv);

  // When uncommented, the data for the data links will be transfered
  // through the files created in the current directory, otherwise
  // the data is passed through the memory.
  gmExperiment.set_link_files(gmFILES_LOCAL | gmFILES_CLEANUP);

  // Set the execution mode
  gmExperiment.set_execution(gmEXE_SERIAL);  // Serial execution where the GridMD calls are ignored
  //gmExperiment.set_execution(gmEXE_CONSTRUCT_ONLY);  // Construction of the execution graph
  //gmExperiment.set_execution(gmEXE_LOCAL);  // Construction of the execution graph and/or
                                            // execution of the selected (all) nodes on the local host
  //gmExperiment.set_execution(gmEXE_REMOTE); // Construction and execution on the remote system(s).
  //                                          // Please set up resources as explained below.
  // Information about external applications, available resource managers and
  // accounts on the remote systems can be stored to XML file (see template in
  // resources.xml). YOU MUST EDIT THIS FILE and specify the appropriate data
  // in order to use 'gmEXE_REMOTE' execution type. Alternatively you can create
  // an instance of gmResourceDescr, fill in the corresponding fields and pass it
  // to the gmExperiment.add_resource() function.
  gmExperiment.load_resources("../resources_valuev.xml");

  // Remove '& (~gmGV_NODESTATE)' to get additional information about node states
  // on the graph generated by graphviz
  gmExperiment.set_graphviz_flags(gmGV_ALL & (~gmGV_NODESTATE));

  // Data types for the data transfer
  typedef double arg_t;   // argument type
  typedef double value_t; // result type
  arg_t x = 1.0;          // argument value for exp(x)
  int nterms = 3;     // number of terms in the series
  int i, fact = 1;
  value_t sum = 0;

  begin_distributed();  // creates 'start' node on the execution graph
  gmFork<void,value_t,void> fork1("loop");  // defines the skeleton and internal data link types
  
  fork1.begin_here();  // creates the loop 'begin' node
  for(i=0; i<nterms; i++){
    fact = i>0 ? fact*i : fact;  // fact is the factorial (denominator for the current term)
     // Create a new 'split' node and defines its output
    if(fork1.split())
      fork1.vsplit_out() = pow(x, (value_t)i) / fact;
    // Define the action of the 'merge' node
    if(fork1.merge())
      sum += fork1.vmerge_in();  // accumulation of the terms
  }

  if( end_distributed() ) // adds 'finish' node and output results
    printf("=== First %d terms of exp(%g) series result in %g\n", nterms, x, sum);

  return 0;
}
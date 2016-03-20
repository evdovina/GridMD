#include "gridmd.h"
#include "gmfork.h"
#include <cmath>
#include <iostream>

double integrate(double a, double b, unsigned long nsub, double (*f)(double) )
{
    double psum = f(a)+f(b);
    double deltaX = (b-a)/nsub;

    for(int index=1;index<nsub;index++)
        psum = psum + 2.0*f(a+index*deltaX);

    psum = (deltaX/2.0)*psum;

    return psum;

}

using namespace gridmd;

int main(int argc,char* argv[]){


  // wxWindows initialization
  if( !gmdInitialize() ) {
    puts("Failed to initialize the GridMD library.");
    return -1;
  }

  // The following command enables output of information messages (vblALLMESS),
  // warnings and errors (vblALLBAD). Remove 'vblALLMESS' to get rid of
  // many information messages during execurion.
  // The program is interrupted on errors (vblERR).
  message_logger::global().set_levels(vblALLBAD | vblALLMESS/*, vblERR*/);

  // All distributed code should be put into gridmd_main() function
  int res= gridmd_main(argc, argv);

  gmdUninitialize();
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
  //gmExperiment.set_execution(gmEXE_SERIAL);  // Serial execution where the GridMD calls are ignored
  //gmExperiment.set_execution(gmEXE_CONSTRUCT_ONLY);  // Construction of the execution graph
  gmExperiment.set_execution(gmEXE_REMOTE);  // Construction of the execution graph and/or
                                            // execution of the selected (all) nodes on the local host
  //gmExperiment.set_execution(gmEXE_REMOTE); // Construction and execution on the remote system(s).
  //                                          // Please set up resources as explained below.
  // Information about external applications, available resource managers and
  // accounts on the remote systems can be stored to XML file (see template in
  // resources.xml). YOU MUST EDIT THIS FILE and specify the appropriate data
  // in order to use 'gmEXE_REMOTE' execution type. Alternatively you can create
  // an instance of gmResourceDescr, fill in the corresponding fields and pass it
  // to the gmExperiment.add_resource() function.
  gmExperiment.load_resources("pbs_libssh.xml");

  // Remove '& (~gmGV_NODESTATE)' to get additional information about node states
  // on the graph generated by graphviz
  gmExperiment.set_graphviz_flags(gmGV_ALL & (~gmGV_NODESTATE));


  double a = 0;
  double b  = 200*M_PI;
  unsigned long nsub = 1E8;
  size_t branchesNum = 8;
  double result = 0;

  double deltaX = (b - a) / double(branchesNum);
  vector<double> branchArgStart(branchesNum + 1);

  branchArgStart.front() = a ;

  for(size_t i = 1; i < branchesNum + 1; ++i)
      branchArgStart[i] = branchArgStart[i - 1] + deltaX;

  begin_distributed();

  gmFork<void,double> fork("numeric_intergarion");

  fork.begin_here();

  for(size_t i = 0; i < branchesNum; ++i){
      if(fork.split()) {
          fork.vsplit_out() = integrate(branchArgStart[i], branchArgStart[i + 1], nsub / branchesNum, sin);
      }
  }

  if(fork.end()) {
      for(size_t i = 0; i < branchesNum; ++i){
              result += fork.vend_in(i);
      }
      std::cout << "Result of integration : " << result << std::endl;
  }

  end_distributed();
  return 0;
}

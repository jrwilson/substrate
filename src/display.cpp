#include <ioa/ioa.hpp>
#include <ioa/global_fifo_scheduler.hpp>

#include <iostream>

class camera :
  public ioa::automaton
{

};

class display :
  public ioa::automaton
{

};

class display_driver :
  public ioa::automaton
{

};

int main (int argc, char* argv[]) {
  ioa::global_fifo_scheduler sched;
  ioa::run (sched, ioa::make_generator<display_driver> ());
  return 0;
}

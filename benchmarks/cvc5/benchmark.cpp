#include "sightglass.h"

#include <cvc5/cvc5.h>
#include <cvc5/cvc5_parser.h>

#include <iostream>

#include "default_smt2.h"

int main(int argc, char** argv)
{
  if (argc > 1)
  {
    std::cerr << "cvc5 benchmark does not accept runtime input files" << std::endl;
    return 1;
  }

  cvc5::TermManager tm;
  cvc5::Solver solver(tm);
  cvc5::parser::InputParser parser(&solver);
  parser.setStringInput(
      cvc5::modes::InputLanguage::SMT_LIB_2_6, kDefaultSmt2, "default.smt2");

  cvc5::parser::SymbolManager* sm = parser.getSymbolManager();

  bench_start();
  while (true)
  {
    cvc5::parser::Command command = parser.nextCommand();
    if (command.isNull())
    {
      break;
    }
    command.invoke(&solver, sm, std::cout);
  }
  bench_end();

  return 0;
}

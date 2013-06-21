//
//  DebuggerContext.cpp
//  Xspray
//
//  Created by Sébastien Métrot on 6/21/13.
//
//

#include "Xspray.h"

namespace Xspray
{

DebuggerContext::DebuggerContext()
: mDebugger(lldb::SBDebugger::Create())
{

}


}
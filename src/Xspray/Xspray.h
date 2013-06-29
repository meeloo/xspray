//
//  Xspray.h
//  Xspray
//
//  Created by Sébastien Métrot on 6/21/13.
//
//

#pragma once

#include "nui.h"

#include <LLDB/LLDB.h>
#include <LLDB/SBStream.h>
#include <LLDB/SBTypeCategory.h>

namespace Xspray
{
#include "Breakpoint.h"
#include "ModuleTree.h"
#include "SourceView.h"
#include "VariableNode.h"
#include "ProcessTree.h"
#include "DebuggerContext.h"
#include "Architecture.h"
}

#include "iOSRemoteDebug.h"

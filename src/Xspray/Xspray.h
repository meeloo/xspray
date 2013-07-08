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

#include "iOSRemoteDebug.h"
#include "NativeFileDialog.h"

namespace Xspray
{
#include "AppDescription.h"
#include "Breakpoint.h"
#include "ModuleTree.h"
#include "SourceView.h"
#include "VariableNode.h"
#include "ProcessTree.h"
#include "DebuggerContext.h"
#include "HomeView.h"
#include "DebugView.h"
#include "ArrayModel.h"
#include "GraphView.h"
}


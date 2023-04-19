#include <stdio.h>

#include <epicsExport.h>
#include <iocsh.h>

#include <asynPortDriver.h>

#include <util_sdb.h>

#include "pcie-single.h"

class BuildInformation: public asynPortDriver {
    int p_name, p_commit, p_tool_name, p_tool_version, p_date, p_user_name;

  public:
    BuildInformation():
      asynPortDriver("BUILD_INFO", 1, -1, -1, 0, 1, 0, 0)
    {
        auto info = get_synthesis_info(&bars);
        auto main_info = info.at(0);

        createParam("NAME", asynParamOctet, &p_name);
        createParam("COMMIT", asynParamOctet, &p_commit);
        createParam("TOOL_NAME", asynParamOctet, &p_tool_name);
        createParam("TOOL_VERSION", asynParamOctet, &p_tool_version);
        createParam("DATE", asynParamOctet, &p_date);
        createParam("USER_NAME", asynParamOctet, &p_user_name);

        char scratch[32];

        setStringParam(p_name, main_info.name);
        setStringParam(p_commit, main_info.commit);
        setStringParam(p_tool_name, main_info.tool_name);

        sprintf(scratch, "0x%08x", main_info.tool_version);
        setStringParam(p_tool_version, scratch);

        sprintf(scratch, "0x%08x", main_info.date);
        setStringParam(p_date, scratch);

        setStringParam(p_user_name, main_info.user_name);
    }
};

static iocshFuncDef func_def {
    "BuildInformation",
    0, NULL
#ifdef IOCSHFUNCDEF_HAS_USAGE
    , "Create a build_info port\n"
#endif
};

static void call_func([[maybe_unused]] const iocshArgBuf *args)
{
    new BuildInformation();
}

static void registerBuildInformation()
{
    iocshRegister(&func_def, call_func);
}

epicsExportRegistrar(registerBuildInformation);

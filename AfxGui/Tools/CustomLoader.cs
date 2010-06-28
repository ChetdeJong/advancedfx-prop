﻿// Copyright (c) advancedfx.org
//
// Last changes:
// 2010-06-28 by dominik.matrixstorm.com
//
// First changes:
// 2009-09-29 by dominik.matrixstorm.com

using System;
using System.Windows.Forms;

namespace AfxGui.Tools {

class CustomLoader
{
    public static bool RunCustomLoader(IWin32Window owner)
    {
	    CfgCustomLoader cfg = GlobalConfig.Instance.Settings.CustomLoader;

        bool bOk;

        using (CustomLoaderForm frm = new CustomLoaderForm())
        {
            frm.Icon = Program.Icon;
            frm.ShowInTaskbar = false;

            frm.HookDll = cfg.HookDllPath;
            frm.Program = cfg.ProgramPath;
            frm.CmdLine = cfg.CmdLine;

            DialogResult dr = frm.ShowDialog(owner);

            if (DialogResult.OK == dr)
            {
                cfg.HookDllPath = frm.HookDll;
                cfg.ProgramPath = frm.Program;
                cfg.CmdLine = frm.CmdLine;

                GlobalConfig.Instance.BackUp();

                bOk = AfxCppCli.AfxHook.LauchAndHook(frm.Program, frm.CmdLine, frm.HookDll);

                if (!bOk)
                    MessageBox.Show("CustomLoader failed", "Error", MessageBoxButtons.OK, MessageBoxIcon.Error);
            }
            else
                bOk = true;
        }

        return bOk;
    }
}

} // namespace AfxGui.Tools {

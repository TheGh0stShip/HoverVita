// Headless: decompile functions at the given hex addresses (args after the output path).
// Usage: -postScript Decompile.java <out.c> 4128f0 419de0 ...
// @category VitaHover
import ghidra.app.script.GhidraScript;
import ghidra.app.cmd.function.CreateFunctionCmd;
import ghidra.app.decompiler.*;
import ghidra.program.model.address.Address;
import ghidra.program.model.listing.Function;
import java.io.*;

public class Decompile extends GhidraScript {
    @Override
    public void run() throws Exception {
        String[] args = getScriptArgs();
        DecompInterface di = new DecompInterface();
        di.openProgram(currentProgram);
        try (PrintWriter out = new PrintWriter(new FileWriter(args[0]))) {
            for (int i = 1; i < args.length; i++) {
                Address a = toAddr(Long.parseLong(args[i], 16));
                Function f = getFunctionContaining(a);
                if (f == null) {
                    disassemble(a);
                    new CreateFunctionCmd(a).applyTo(currentProgram);
                    f = getFunctionAt(a);
                }
                if (f == null) { out.println("/* no function at " + a + " */"); continue; }
                DecompileResults r = di.decompileFunction(f, 120, monitor);
                out.printf("/* %s @ %s */%n", f.getName(true), f.getEntryPoint());
                out.println(r.decompileCompleted() ? r.getDecompiledFunction().getC()
                        : "/* decompile failed: " + r.getErrorMessage() + " */");
            }
        }
    }
}

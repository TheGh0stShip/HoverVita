// Dumps every function's decompiled C into one file, plus a function list.
import ghidra.app.script.GhidraScript;
import ghidra.app.decompiler.*;
import ghidra.program.model.listing.*;
import java.io.*;

public class ExportDecomp extends GhidraScript {
    @Override
    public void run() throws Exception {
        String outDir = getScriptArgs()[0];
        DecompInterface di = new DecompInterface();
        di.openProgram(currentProgram);
        try (PrintWriter c = new PrintWriter(new File(outDir, "hover_decomp.c"));
             PrintWriter l = new PrintWriter(new File(outDir, "functions.txt"))) {
            for (Function f : currentProgram.getFunctionManager().getFunctions(true)) {
                l.printf("%s %6d %s%n", f.getEntryPoint(), f.getBody().getNumAddresses(), f.getName());
                DecompileResults r = di.decompileFunction(f, 60, monitor);
                c.printf("/* %s @ %s */%n", f.getName(), f.getEntryPoint());
                c.println(r.decompileCompleted() ? r.getDecompiledFunction().getC() : "/* decompile failed */");
            }
        }
    }
}

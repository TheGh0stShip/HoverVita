// Applies re/symbols.csv (address,kind,name) to the current program.
// kind: func -> create/rename a function; data -> label. "A::b" puts b in namespace A.
// @category VitaHover
import ghidra.app.script.GhidraScript;
import ghidra.app.cmd.function.CreateFunctionCmd;
import ghidra.app.util.NamespaceUtils;
import ghidra.program.model.address.Address;
import ghidra.program.model.listing.Function;
import ghidra.program.model.symbol.*;
import java.io.*;
import java.nio.file.*;

public class ApplySymbols extends GhidraScript {
    @Override
    public void run() throws Exception {
        Path csv = Paths.get(getScriptArgs().length > 0 ? getScriptArgs()[0]
                : askFile("symbols.csv", "Apply").getPath());
        SymbolTable st = currentProgram.getSymbolTable();
        int n = 0;
        for (String line : Files.readAllLines(csv)) {
            line = line.strip();
            if (line.isEmpty() || line.startsWith("#") || line.startsWith("address,")) continue;
            String[] f = line.split(",", 3);
            Address a = toAddr(Long.parseLong(f[0], 16));
            String full = f[2].strip();
            Namespace ns = currentProgram.getGlobalNamespace();
            int sep = full.lastIndexOf("::");
            String name = full;
            if (sep > 0) {
                ns = NamespaceUtils.createNamespaceHierarchy(full.substring(0, sep), null,
                        currentProgram, SourceType.USER_DEFINED);
                name = full.substring(sep + 2);
            }
            if (f[1].equals("func")) {
                Function fn = getFunctionAt(a);
                if (fn == null) {
                    disassemble(a);
                    new CreateFunctionCmd(a).applyTo(currentProgram);
                    fn = getFunctionAt(a);
                }
                if (fn == null) { printerr("could not create function at " + a); continue; }
                fn.setParentNamespace(ns);
                fn.setName(name, SourceType.USER_DEFINED);
            } else {
                Symbol s = st.createLabel(a, name, ns, SourceType.USER_DEFINED);
                s.setPrimary();
            }
            n++;
        }
        println("Applied " + n + " symbols from " + csv);
    }
}

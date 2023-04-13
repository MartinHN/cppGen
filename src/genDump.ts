import { APIClass, TypeSystem } from "./APILoader";
import * as fs from "fs"
import * as tmpl from "./parseTemplate"


 export function genDump(typeSyst: TypeSystem, outFolder: string): string[] {

    const dumpF = outFolder+"/dump";
    if  (!fs.existsSync(dumpF)){ fs.mkdirSync(dumpF);}


    let dumpFiles = []

    for (const cl of typeSyst.classes) {
      const localName = cl.name + "_DebugDump.h";
      const fN = dumpF + "/" + localName;
      const o = tmpl.parseTemplate("DebugDump.ejs", cl)
      fs.writeFileSync(fN, o);
      dumpFiles.push(localName);
    }

   if (dumpFiles.length) {
     const localName = "prelude_DebugDump.h";
     const fN = dumpF + "/" + localName;
     const o = tmpl.parseTemplate("DebugDump_prelude.ejs", {})
     fs.writeFileSync(fN, o);
     dumpFiles.unshift(localName);

   }
  const  dumpFMain = dumpF+"/dump.h"
  fs.writeFileSync(dumpFMain,dumpFiles.map(e=>`#include "${e}"`).join("\n"));
    return [dumpFMain];


}

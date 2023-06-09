import { APIClass, TypeSystem } from "./APILoader";
import * as fs from "fs"
import * as tmpl from "./parseTemplate"

export function genSerialize(typeSyst: TypeSystem, outFolder: string): string[] {
    return genSerializeBin(typeSyst, outFolder)
}


 function genSerializeBin(typeSyst: TypeSystem, outFolder: string): string[] {

    const moduleFolder = outFolder+"/serialize";
    if  (!fs.existsSync(moduleFolder)){ fs.mkdirSync(moduleFolder);}
    let localFiles = []
    for (const cl of typeSyst.classes) {
        const localName = cl.name+"_serializeBin.h";
        const fN = moduleFolder+"/"+localName;
        const o = tmpl.parseTemplate("serializeBin.ejs", cl)
        fs.writeFileSync(fN,o);
        localFiles.push(localName);
    }

     if (localFiles.length) {
         const localName = "serializeBin_prelude.h";
         const fN = moduleFolder + "/" + localName;
         const o = tmpl.parseTemplate("serializeBin_prelude.cpp", {})
         fs.writeFileSync(fN, o);
         localFiles.unshift(localName);

     }
    const moduleH = moduleFolder+"/serialize.h";
    fs.writeFileSync(moduleH,localFiles.map(e=>`#include "${e}"`).join("\n"));

    return [moduleH];


}

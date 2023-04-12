import { APIClass, TypeSystem } from "./APILoader";
import * as fs from "fs"
import * as tmpl from "./parseTemplate"



export function genProxy(typeSyst: TypeSystem, outFolder: string): string[] {

    const moduleFolder = outFolder + "/proxy";
    if (!fs.existsSync(moduleFolder)) { fs.mkdirSync(moduleFolder); }
    let localFiles = []
    for (const cl of typeSyst.classes) {
        const localName = cl.name + "_proxy.h";
        const fN = moduleFolder + "/" + localName;
        const o = tmpl.parseTemplate("proxy.ejs", cl)
        fs.writeFileSync(fN, o);
        localFiles.push(localName);
    }

    const moduleH = moduleFolder + "/proxy.h";
    fs.writeFileSync(moduleH, localFiles.map(e => `#include "${e}"`).join("\n"));

    return [moduleH];


}

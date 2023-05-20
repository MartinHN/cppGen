import { APIClass, MemberType, MethodType, TypeSystem } from "./APILoader";
import * as fs from "fs"
import * as tmpl from "./parseTemplate"

export function genJsBind(typeSyst: TypeSystem, outFolder: string): string[] {
    const moduleFolder = outFolder + "/jsBind";
    if (!fs.existsSync(moduleFolder)) { fs.mkdirSync(moduleFolder); }
    let localFiles = []
    for (const cl of typeSyst.classes) {
        const localName = cl.name + "_jsBind.h";
        const fN = moduleFolder + "/" + localName;
        const o = tmpl.parseTemplate("JsBind.ejs", cl)
        fs.writeFileSync(fN, o);
        localFiles.push(localName);
    }

    if (localFiles.length) {
        const localName = "JsBind_prelude.h";
        const fN = moduleFolder + "/" + localName;
        const o = tmpl.parseTemplate("JsBind_prelude.cpp", {})
        fs.writeFileSync(fN, o);
        localFiles.unshift(localName);

    }

    const moduleH = moduleFolder + "/JsBind.h";
    fs.writeFileSync(moduleH, localFiles.map(e => `#include "${e}"`).join("\n"));

    return [moduleH];
}


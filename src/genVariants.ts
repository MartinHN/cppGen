import { TypeSystem, APIClass } from "./APILoader";
import * as fs from "fs"
import * as tmpl from './parseTemplate'


export function genVariants(ts: TypeSystem, outFolder: string): string[] {

    const outFile = outFolder + "/variants.h"
    const allCls = ts.getAllClasses();

    const ctx = { builtins: ["uint32_t", "unsigned char"], classes: allCls.map(i => { return { name: i, members: ts.getMembersForClass(i), methods: ts.getMethodsForClass(i), isUserDefined: ts.isClassUserDefined(i) } }) }

    const o = tmpl.parseTemplate("Variants.ejs", ctx)
    fs.writeFileSync(outFile, o);

    return [outFile]
}

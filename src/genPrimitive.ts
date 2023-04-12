import * as tmpl from "./parseTemplate"
import * as fs from 'fs'

export class PrimType {

    public CapName: string;
    public StrName: string;
    public Arithmetic = false;
    constructor(public CName: string) {
        const niceN = CName.replace("::", "_");
        this.CapName = niceN[0].toUpperCase() + niceN.substring(1);
        this.StrName = niceN;
    }


    getOverrides() {

        return this;
    }
}




export function genPrimTypes(outFolder: string) {
    const added: string[] = [];
    // const arithmL: PrimType[] = [new PrimType("int"), new PrimType("float"), new PrimType("double")]
    // arithmL.map(e => e.Arithmetic = true);
    // const primList = arithmL
    // primList.push(new PrimType("std::string"))
    // console.log("gen prim types", primList.map(e => e.CName))

    // for (const p of primList) {
    //     const outF = outFolder + "/Prim_" + p.CapName + ".h"
    //     const o = tmpl.parseTemplate("PrimitiveType.ejs", p.getOverrides())
    //     fs.writeFileSync(outF, o);
    //     added.push(outF)
    // }

    return added;
}

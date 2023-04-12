import {  TypeSystem } from "./APILoader";
import * as fs from "fs"
import * as tmpl from './parseTemplate'


export function genIdentifiers(ts:TypeSystem,outFolder:string) : string[]{
    
   const  outFile= outFolder+"/identifierPool.h"
    const allIds = ts.getAllIds();
    const ctx = {identifiers:allIds.map(i=>{return {name:i,shortKey:allIds.indexOf(i)}})}
    const o = tmpl.parseTemplate("Identifiers.ejs", ctx)
    fs.writeFileSync(outFile,o);

    return [outFile]
}

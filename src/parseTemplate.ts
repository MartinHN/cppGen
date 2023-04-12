import * as fs from "fs"
import * as ejs from "ejs"
import * as path from "path"

let parsed = new Array<string>();

const rootTemplDir =__dirname+"/tmpl/"
export function parseTemplate(fileName: string, overrides: any): string {
    const fullFileName = rootTemplDir+fileName
    let tp = fs.readFileSync(fullFileName).toString();
    const ctx={init:parsed.find(e=>e==fullFileName)===undefined};
    parsed.push(fullFileName)
    return ejs.render(tp,{ctx,...overrides})
   
}

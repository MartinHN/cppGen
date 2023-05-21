import * as prim from "./genPrimitive"
import * as apiLoader from "./APILoader"
import { genDump } from "./genDump"
import { genSerialize } from "./genSerialize"
import { genIdentifiers } from "./genIdentifiers"
import { genVariants } from "./genVariants"
import { genJs } from "./genJs"
import { genProxy } from "./genProxy"
import { execSync } from "child_process"
import * as fs from "fs"
import * as path from "path"

import { genProto } from "./genProto"




function safeClean(of: string) {
    if (of.length < 10) {
        throw Error("folder path to clean is too short?" + of)
    }
    if (fs.existsSync(of)) { execSync("rm -r " + of); }
    execSync("mkdir -p " + of);
}


function safeCpFolder(inF: string, outF: string) {
    console.log("will copy " + inF + " to " + outF)
    if (path.dirname(inF) == path.dirname(outF)) {
        console.log("not copying to same dir")
        return;
    }
    // safeClean(outF)
    execSync("cp -r " + inF + " " + outF);
}


export function genAll(jsonPath: string, outFolder: string, outJsFolder?: string) {
    safeClean(outFolder)
    const localGenPath = __dirname + "/common"
    safeCpFolder(localGenPath, outFolder)
    ///////// 
    // added prim
    const addedFiles = new Array<string>();
    //primFiles=prim.genPrimTypes(outFolder)


    ///////// 
    // user defined classes

    const api = apiLoader.load(jsonPath);

    let allIncludes = new Array<string>()

    console.log(api)

    const dumpFiles: string[] = genDump(api, outFolder)
    allIncludes = allIncludes.concat(dumpFiles);
    const serializedFiles: string[] = genSerialize(api, outFolder)
    allIncludes = allIncludes.concat(serializedFiles);
    const idFiles: string[] = genIdentifiers(api, outFolder)
    allIncludes = allIncludes.concat(idFiles);
    const variantsFiles: string[] = genVariants(api, outFolder)
    allIncludes = allIncludes.concat(variantsFiles);
    const proxyFiles = genProxy(api, outFolder);
    allIncludes = allIncludes.concat(proxyFiles);



    let res = "// main header file\n"
    res += `#if !HAS_CUSTOM_API_INCLUDED\n// original API File\n#include "${api.metadata?.originalFile}"\n#endif\n`
    res += "#include \"./common/common.h\"\n"
    allIncludes.map(e => res += `#include "${path.relative(outFolder, e)}"\n`)
    res += "#include \"./common/messageHelps.h\"\n";


    fs.writeFileSync(outFolder + "/gen.h", res);

    // genProto(api, outFolder);
    const lintRes = execSync(`find ${outFolder} -iname "*.h" -o -iname "*.cpp" -o -iname "*.proto" | xargs clang-format -style="{SortIncludes: Never}" -i`)

    let jslintRes = Buffer.from("");
    if (!!outJsFolder) {
        safeClean(outJsFolder)
        genJs(api, outJsFolder)


        const lintCmd = `find ${outJsFolder} -iname "*.js" -o -iname "*.ts" ! -iname '*prelude.ts' | xargs clang-format -style="{SortIncludes: Never}" -i`
        // console.log(lintCmd);
        jslintRes = execSync(lintCmd)
    }
    console.log("SUCCCCCESSSS", lintRes.toString(), jslintRes.toString())

    return api;
}

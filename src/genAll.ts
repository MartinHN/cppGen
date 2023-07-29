// import * as prim from "./genPrimitive"
import * as apiLoader from "./APILoader"
import { genDump } from "./genDump"
import { genSerialize } from "./genSerialize"
import { genIdentifiers } from "./genIdentifiers"
import { genVariants } from "./genVariants"
import { genJs } from "./genJs"
import { genProxy } from "./genProxy"
import { execSync, spawn, spawnSync } from "child_process"
import * as fs from "fs"
import * as path from "path"

// import { genProto } from "./genProto"
import { genWasm } from "./genWasm"
import { exit } from "process"
import { mkSymLink, rsync, safeClean, safeCpFolder } from "./fileHelps"




export async function genAll(jsonPath: string, trueOutFolder: string, opts?: { useSymlinks?: boolean }, trueOutJsFolder?: string, wasmOpts?: any) {
    const outFolder = "/tmp/genCpp/"
    opts = opts || {}

    safeClean(outFolder)


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
    // common files
    const localCommonPath = __dirname + "/common"
    if (!opts.useSymlinks)
        safeCpFolder(localCommonPath, outFolder)
    rsync(outFolder, trueOutFolder)

    if (opts.useSymlinks) {
        const trueCommon = trueOutFolder + "/" + path.basename(localCommonPath);
        if (fs.existsSync(trueCommon)) {
            const s = fs.statSync(trueCommon)
            if (s.isDirectory()) {
                fs.rmSync(trueCommon, { recursive: true });
            }
            else if (s.isSymbolicLink()) {
                fs.unlinkSync(trueCommon);
            }
            else {
                throw new Error("unknown common folder type")
            }
        }
        mkSymLink(localCommonPath, trueOutFolder)
    }


    if (!!trueOutJsFolder) {

        const outJsFolder = "/tmp/genJs/"
        safeClean(outJsFolder)
        genJs(api, outJsFolder)


        const lintCmd = `find ${outJsFolder} -iname "*.js" -o -iname "*.ts" ! -iname '*prelude.ts' | xargs clang-format -style="{SortIncludes: Never}" -i`
        // console.log(lintCmd);
        jslintRes = execSync(lintCmd)

        rsync(outJsFolder, trueOutJsFolder)
        if (wasmOpts)
            await genWasm(api, trueOutFolder, wasmOpts.baseClass, trueOutJsFolder, wasmOpts.debugMode, wasmOpts.buildForNode, opts)


    }
    console.log("SUCCCCCESSSS", lintRes.toString(), jslintRes.toString())

    return api;
}



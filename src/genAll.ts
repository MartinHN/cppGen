import * as prim from "./genPrimitive"
import * as apiLoader from "./APILoader"
import { genDump } from "./genDump"
import { genSerialize } from "./genSerialize"
import { genIdentifiers } from "./genIdentifiers"
import { genVariants } from "./genVariants"
import { genProxy } from "./genProxy"
import { execSync } from "child_process"
import * as fs from "fs"
import * as path from "path"



function safeClean(of: string) {
    if (of.length < 10) {
        throw Error("folder path to clean is too short?" + of)
    }
    execSync("rm -r " + of);
    execSync("mkdir " + of);
}



export function genAll(jsonPath: string, outFolder: string) {
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
    res += "#include \"../common/common.h\"\n"
    allIncludes.map(e => res += `#include "${path.relative(outFolder, e)}"\n`)
    res += "#include \"../common/messageHelps.h\"\n";
    fs.writeFileSync(outFolder + "/gen.h", res);


    const lintRes = execSync(`find ${outFolder} -iname *.h -o -iname *.cpp | xargs clang-format -style="{SortIncludes: Never}" -i`)

    console.log("SUCCCCCESSSS", lintRes.toString())
}

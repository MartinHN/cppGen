import * as prim from "./src/genPrimitive"
import * as apiLoader from "./src/APILoader"
import { genDump } from "./src/genDump"
import { genSerialize } from "./src/genSerialize"
import { genIdentifiers } from "./src/genIdentifiers"
import { genVariants } from "./src/genVariants"
import { genProxy } from "./src/genProxy"
import { execSync } from "child_process"
import * as fs from "fs"
import * as path from "path"

const outFolder = "./gen"

execSync("rm -rf " + outFolder);
execSync("mkdir " + outFolder);


///////// 
// added prim
const addedFiles = new Array<string>();
//primFiles=prim.genPrimTypes(outFolder)


///////// 
// user defined classes
const jsonPath = "/Users/tinmarbook/Dev/UAPI/genJSONSchema/test/gen/RootAPI.json";
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

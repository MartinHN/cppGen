
import { genAll } from "./src/genAll"
import * as path from "path"

let outFolder = path.resolve("./gen")
let outJsFolder = outFolder + "/js"
const jsonPath = "/Users/tinmarbook/Dev/UAPI/genJSONSchema/test/gen/RootAPI.json";
genAll(jsonPath, outFolder, {}, outJsFolder)

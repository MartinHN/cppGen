
import { genAll } from "./src/genAll"
import * as path from "path"

let outFolder = path.resolve("./gen")
const jsonPath = "/Users/tinmarbook/Dev/UAPI/genJSONSchema/test/gen/RootAPI.json";
genAll(jsonPath, outFolder)

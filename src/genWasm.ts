import { TypeSystem, APIClass } from "./APILoader";
import { spawn } from "child_process";


async function runCmd(cmd: string) {
    const child = spawn(cmd, { shell: true })
    child.stdout.setEncoding('utf8');
    child.stdout.on('data', console.log)
    child.stderr.setEncoding('utf8');
    child.stderr.on('data', console.error)
    const exitCode = await new Promise((resolve, reject) => {
        child.on('close', resolve);
    });

}
export async function genWasm(ts: TypeSystem, cppFolder: string, jsBaseClass: string, outFolder: string, debugMode?: boolean, buildForNode?: boolean) {
    const buildDest = `${outFolder}/build`
    const dbgFlag = "-DCMAKE_BUILD_TYPE=" + (debugMode ? "Debug" : "Release")
    const emsdkPath = process.env["EMSDK"]
    if (!emsdkPath)
        throw new Error("emsdkpath should be set")
    const EMSDK_TOOLS = "-DCMAKE_TOOLCHAIN_FILE=$EMSDK/upstream/emscripten/cmake/Modules/Platform/Emscripten.cmake"

    const cmakeDirVars = `-DCMAKE_RUNTIME_OUTPUT_DIRECTORY=${outFolder}/wasm -DGEN_DIR=${cppFolder}`
    const cmList = __dirname + `/wasm/`
    let uapiDefs = `-DROOT_JS_CLASS=${jsBaseClass} -DBUILD_TYPESCRIPT=1 -DZIP_ASSETS=1 `
    if (buildForNode)
        uapiDefs += "-DBUILD_FOR_NODE=1"

    const configCmd = `cmake -B ${buildDest} -S ${cmList} ${cmakeDirVars} ${dbgFlag} ${EMSDK_TOOLS} ${uapiDefs}`
    await runCmd(configCmd)

    const buildCmd = `cmake --build ${buildDest} `
    await runCmd(buildCmd)

}

import { APIClass, MemberType, MethodType, TypeSystem } from "./APILoader";
import * as fs from "fs"
import * as tmpl from "./parseTemplate"

export function genEmBind(typeSyst: TypeSystem, outFolder: string): string[] {
    const moduleFolder = outFolder + "/embind";
    if (!fs.existsSync(moduleFolder)) { fs.mkdirSync(moduleFolder); }
    let localFiles = []
    for (const cl of typeSyst.classes) {
        const localName = cl.name + "_embind.h";
        const fN = moduleFolder + "/" + localName;
        const o = tmpl.parseTemplate("Embind.ejs", cl)
        fs.writeFileSync(fN, o);
        localFiles.push(localName);
    }

    if (localFiles.length) {
        const localName = "Embind_prelude.h";
        const fN = moduleFolder + "/" + localName;
        const o = tmpl.parseTemplate("Embind_prelude.cpp", {})
        fs.writeFileSync(fN, o);
        localFiles.unshift(localName);

    }

    const moduleH = moduleFolder + "/embind.h";
    fs.writeFileSync(moduleH, localFiles.map(e => `#include "${e}"`).join("\n"));

    return [moduleH];
}

/*

TODO generate typescript
 this is tsembind.ts modified to accept module

import path from 'path'
import { injectBindings } from './injection/inject'

import { WASMExports } from './wasm'
import { EmscriptenModule } from './emscripten'
import { Registry } from './injection/registry'
import { emptyHintFunction as emptyAnnotator, Annotator as Annotator, annotateRegistry } from './annotation'
import { convertInjectionRegistryToDeclarationRegistry } from './declaration'
import { declarationsForRegistry } from './declaration/generate'

const registries = new Map<WASMExports, Registry>();

const wrapWebAssemblyInit =
    (init: CallableFunction) =>
        async (binary: WASMExports, info: WebAssembly.ModuleImports) => {
            const { registry, injectedInfo } = injectBindings(info)
            const result = await init(binary, injectedInfo)
            const { module, instance } = result;
            registries.set(instance.exports, registry)
            return result;
        }

// TODO fix typing with something less hacky
(WebAssembly.instantiate as any) = wrapWebAssemblyInit(WebAssembly.instantiate)

// note that the Emscripten Module is NOT the WebAssembly Module.
// However, since they share some components, we can find a mapping
const registryForEmscriptenModule = (module: EmscriptenModule) =>
    registries.get(module.asm)

const getDeclarations = (module: EmscriptenModule, annotator: Annotator) => {
    const injectionRegistry = registryForEmscriptenModule(module);
    if (!injectionRegistry)
        throw new Error("Cannot find module")

    const declarationRegistry =
        convertInjectionRegistryToDeclarationRegistry(injectionRegistry, module)

    const annotated = annotateRegistry(declarationRegistry, annotator)

    return declarationsForRegistry(annotated)
}

interface GlobalEmscriptenModule { onRuntimeInitialized: any }
async function moduleFromRequire(
    requireResult: CallableFunction | GlobalEmscriptenModule
) {
    if (typeof requireResult === "function")
        return await requireResult();
    else {
        await new Promise(res => requireResult.onRuntimeInitialized = res)
        return requireResult
    }
}


export async function generateTypescriptBindings(
    inputFilename: string, annotator?: Annotator
) {
    const absoluteInputFilename = path.resolve(process.cwd(), inputFilename)
    const module = await (await import(absoluteInputFilename)).default()
    // console.log(module)
    // const module = await moduleFromRequire(require(absoluteInputFilename))
    return getDeclarations(module, annotator || emptyAnnotator)
}
*/

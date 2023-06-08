import { APIClass, MemberType, MethodType, TypeSystem } from "./APILoader";
import * as fs from "fs"
import * as tmpl from "./parseTemplate"

export function genJs(typeSyst: TypeSystem, outFolder: string): string[] {
    if (!fs.existsSync(outFolder)) { fs.mkdirSync(outFolder); }
    return genJsDecl(typeSyst, outFolder + "/decl");
}


function getInnerTemplate(oriT: string) {
    return "" + oriT.substring(oriT.indexOf("<") + 1, oriT.lastIndexOf(">"));
}

function getJsTypeForCType(typeSyst: TypeSystem, t: string, accessed: { [key: string]: Set<string> }): string {
    if (t.includes("*") || t.includes("&")) {
        throw new Error("pointers and ref not supported for now")
        return "unsupported"
    }
    if (typeSyst.isClassUserDefined(t)) {
        const fName = t + "_Decl"
        if (!accessed[fName]) {
            accessed[fName] = new Set<string>()
        }
        let d = accessed[fName]

        const iName = t + "_Interface";
        const dName = t + "_Data";
        d?.add(iName);
        d?.add(dName);
        return iName;
    }

    if (t.toLowerCase().startsWith("void"))
        return "void"

    if (t == "bool")
        return "boolean"
    const rawT = t.replace("unsigned", "").trim();
    if (rawT.startsWith("int") || rawT.startsWith("float") || rawT.startsWith("short") || rawT.startsWith("double"))
        return "number"
    if (rawT.startsWith("long"))
        return "BigInt"
    if (rawT.startsWith("std::string"))
        return "string"

    if (rawT.startsWith("std::vector")) {
        const innerT = getInnerTemplate(rawT);
        return "ModifiableArray<" + getJsTypeForCType(typeSyst, innerT, accessed) + ">";
    }
    if (rawT.startsWith("std::array")) {
        const innerT = getInnerTemplate(rawT).split(",");
        return "FixedLengthArray<" + getJsTypeForCType(typeSyst, innerT[0], accessed) + "," + innerT[1] + ">";
    }

    console.log("not found ts type for", t)
    return "unsupported"
}



function appendJsTypes(typeSyst: TypeSystem, cl: APIClass) {
    const userClassesAccessed = {} as { [key: string]: Set<string> }
    const jsClass = new APIClass(cl.name, cl);
    jsClass.members = new Array<MemberType>();
    for (let o of cl.members) {
        jsClass.members.push(new MemberType(getJsTypeForCType(typeSyst, o.type, userClassesAccessed), o.name, o.init));
    }

    jsClass.methods = new Array<MethodType>();
    for (const o of cl.methods) {
        const retT = getJsTypeForCType(typeSyst, o.returnType, userClassesAccessed);
        const argsT = o.args.map(e => { return new MemberType(getJsTypeForCType(typeSyst, e.type, userClassesAccessed), e.name); });
        jsClass.methods.push(new JSMethod(retT, o.name, argsT))
    }
    return { userClassesAccessed, jsClass };
}


class JSMethod extends MethodType {
    getFunctionArgsWithVariables() {
        return this.getArgTypeList().map((e, i) => { return this.getArgVarName(i) + ":" + e });
    }
}

export function genJsDecl(typeSyst: TypeSystem, outFolder: string): string[] {
    const jsFolder = outFolder;
    if (!fs.existsSync(jsFolder)) { fs.mkdirSync(jsFolder); }


    let localFiles = []

    for (const cl of typeSyst.classes) {
        const { userClassesAccessed, jsClass } = appendJsTypes(typeSyst, cl);
        const localName = jsClass.name + "_Decl.ts";
        const fN = jsFolder + "/" + localName;
        (jsClass as any).typeSystem = typeSyst;
        (jsClass as any).classesToInclude = userClassesAccessed
        const o = tmpl.parseTemplate("JsDecl.ejs", jsClass)

        fs.writeFileSync(fN, o);
        localFiles.push(localName);
    }
    if (localFiles.length) {
        const localName = "JsDecl_prelude.ts";
        const fN = jsFolder + "/" + localName;
        const o = tmpl.parseTemplate("JsDecl_prelude.ts", {})
        fs.writeFileSync(fN, o);
        localFiles.unshift(localName);
    }

    const jsFolderMain = jsFolder + "/main.ts"
    fs.writeFileSync(jsFolderMain, localFiles.map(e => `export * as ${e.split("_")[0]} from "./${e}"`).join("\n"));
    return [jsFolderMain];
}



import { APIClass, MemberType, MethodType, TypeSystem } from "./APILoader";
import * as fs from "fs"
import * as tmpl from "./parseTemplate"
import { execSync } from "child_process";



const builtinMsgs = {
    uint32_Msg: [new MemberType("uint32", "data")]
}


function getInnerTemplate(oriT: string) {
    return "" + oriT.substring(oriT.indexOf("<") + 1, oriT.lastIndexOf(">"));
}

function getProtoTypeForCType(typeSyst: TypeSystem, t: string, accessed: { [key: string]: Array<MemberType> }): string {
    if (t.includes("*") || t.includes("&")) {
        throw new Error("pointers and ref not supported for now")
        return "unsupported"
    }

    if (typeSyst.isClassUserDefined(t)) {
        return t + "_data";
    }


    if (t.toLowerCase().startsWith("void"))
        return "void"

    const rawT = t.replace("unsigned", "").trim(); // TODO
    // rawT.startsWith("float") ||
    if (rawT.startsWith("int") || rawT.startsWith("short"))
        return "int32"
    if (rawT.startsWith("long"))
        return "int64"
    if (rawT.startsWith("std::string"))
        return "string"

    const arrPrefix = "vec_";//asMsgName ? "vec_" : "repeated "
    if (rawT.startsWith("std::vector")) {
        const innerT = getInnerTemplate(rawT);
        return arrPrefix + getProtoTypeForCType(typeSyst, innerT, accessed);
    }
    if (rawT.startsWith("std::array")) {
        const innerT = getInnerTemplate(rawT).split(",");
        return arrPrefix + getProtoTypeForCType(typeSyst, innerT[0], accessed);
        // return "FixedLengthArray<" + getProtoTypeForCType(typeSyst, innerT[0], accessed) + "," + innerT[1] + ">";
    }
    if (rawT.startsWith("uint"))
        return rawT.replace("_t", "")

    throw Error("not found ts type for " + t)
    return "unsupported"
}

function appendProtoTypes(typeSyst: TypeSystem, cl: APIClass) {
    const commonMessageTypes = {} as { [key: string]: Array<MemberType> }
    const protoClass = new APIClass(cl.name, cl);
    protoClass.members = new Array<MemberType>();
    for (let o of cl.members) {
        const pt = getProtoTypeForCType(typeSyst, o.type, commonMessageTypes);
        protoClass.members.push(new MemberType(pt, o.name));
        let msgName = (pt.length == 0 ? "Void" : pt) + "_Msg"
        if (!commonMessageTypes[msgName]) {
            commonMessageTypes[msgName] = [new MemberType(pt, "data")]
        }
    }

    commonMessageTypes[cl.name + "_data"] = cl.members.map(m => {
        const pt = getProtoTypeForCType(typeSyst, m.type, commonMessageTypes);
        return new MemberType(pt, m.name);
    })
    protoClass.methods = new Array<MethodType>();
    for (const o of cl.methods) {
        const retT = getProtoTypeForCType(typeSyst, o.returnType, commonMessageTypes);
        const argsT = o.args.map((e, i) => { return new MemberType(getProtoTypeForCType(typeSyst, e.type, commonMessageTypes), e.name ? e.name : "data" + i); });
        protoClass.methods.push(new ProtoMethod(retT, o.name, argsT))
        const pt = argsT.map(e => e.name).join("_");
        console.log(argsT, ">>>ll>>", JSON.stringify(o))
        const msgName = (pt.length == 0 ? "Void" : pt) + "_Msg"
        if (!commonMessageTypes[msgName]) {
            commonMessageTypes[msgName] = argsT;
        }
    }
    return { commonMessageTypes, protoClass };
}


class ProtoMethod extends MethodType {
    getFunctionArgsMessage() {
        console.log("calling proto")
        return this.getArgTypeList().map((e, i) => { return this.getArgVarName(i) + ":" + e });
    }
}


export function genProto(typeSyst: TypeSystem, outFolder: string): string[] {

    const moduleFolder = outFolder + "/proto";
    if (!fs.existsSync(moduleFolder)) { fs.mkdirSync(moduleFolder); }
    let localFiles = []
    let allMessageTypes = {} as any;
    allMessageTypes.Void_Msg = [];
    for (const cl of typeSyst.classes) {
        const { commonMessageTypes, protoClass } = appendProtoTypes(typeSyst, cl);
        allMessageTypes = { ...allMessageTypes, ...commonMessageTypes }
        const localName = cl.name + ".proto";
        const fN = moduleFolder + "/" + localName;
        const o = tmpl.parseTemplate("Protobuf.ejs", { cl: protoClass, msgs: commonMessageTypes })
        fs.writeFileSync(fN, o);
        localFiles.push(localName);
    }

    if (localFiles.length) {
        const localName = "prelude.proto";
        const fN = moduleFolder + "/" + localName;
        const commonMessages = allMessageTypes
        Object.entries(builtinMsgs).map(([k, v]) => commonMessages[k] = v);
        //  Object.fromEntries(Object.entries(allMessageTypes).filter(([k]) => {
        //     const ct = k.replaceAll("vec_", "").replaceAll("_Msg", "");
        //     console.log(k, ct)
        //     return true;//!typeSyst.isClassUserDefined(ct);
        // }));
        console.log(">>>>>u", commonMessages)
        const o = tmpl.parseTemplate("Protobuf_prelude.ejs", { commonMessages })
        fs.writeFileSync(fN, o);
        localFiles.unshift(localName);

    }

    const protoCFolder = outFolder + "/protocpp";
    const protoTsFolder = outFolder + "/protots";

    if (!fs.existsSync(protoCFolder)) { fs.mkdirSync(protoCFolder); }
    if (!fs.existsSync(protoTsFolder)) { fs.mkdirSync(protoTsFolder); }
    for (const pf of localFiles) {
        let protoCmd = `cd ${moduleFolder};`;
        protoCmd += `protoc --grpc_out=${protoCFolder} --plugin=protoc-gen-grpc=$(which grpc_cpp_plugin) ${pf};`;
        protoCmd += `protoc --cpp_out=${protoCFolder} ${pf};`
        protoCmd += `protoc --ts_out=${protoTsFolder} --ts_opt=no_grpc ${pf};`;
        execSync(protoCmd)
    }

    const moduleH = protoCFolder + "/root.h";
    fs.writeFileSync(moduleH, localFiles.map(e => `#include "${e.replace(".proto", ".grpc.pb.h")}"`).join("\n"));

    const moduleC = protoCFolder + "/root.cpp";
    fs.writeFileSync(moduleC, localFiles.map(e => `#include "${e.replace(".proto", ".pb.cc")}"`).join("\n") + "\n"
        + localFiles.map(e => `#include "${e.replace(".proto", ".grpc.pb.cc")}"`).join("\n"));

    return [moduleH];


}


export function genProtoJs(typeSyst: TypeSystem, outFolder: string) {
    let protoCmd = `cd ${outFolder};`;



    execSync(protoCmd)
}

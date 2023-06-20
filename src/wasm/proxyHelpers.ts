let root = null as any;

import { MainBuilder, JsWasmTransport } from "./wasmJsTypes"


export class JsCliSender {

    constructor(public msgBuild: MainBuilder, public wasmWriter: JsWasmTransport, public serverSender?: JsWasmTransport) { }
    onJsElementSet(parentAddr: string[], onTo: any, k: string, nV: any): void {
        const msg = this.msgBuild.buildJsBindModMessage(parentAddr.concat(k).join("/"), nV);
        this.wasmWriter.processMsg(msg);
        if (this.serverSender)
            this.serverSender.processMsg(msg);
    }
    onJsElementCall(parentAddr: string[], args: any[]): void {
        const fName = parentAddr.pop()
        const msg = this.msgBuild.buildJsBindCallMessage(parentAddr.join("/"), fName, args);
        this.wasmWriter.processMsg(msg);
        if (this.serverSender)
            this.serverSender.processMsg(msg);
    }

    onNewObjAdded(parentAddr: string[], newObj: any) {
        // TODO
    }
}

let hdlr = undefined as (JsCliSender | undefined);


function elementIsSet(parentAddr: string[], onTo: any, k: string, nV: any) {
    if (root.__fromServer)
        return;
    if (hdlr)
        hdlr.onJsElementSet(parentAddr, onTo, k, nV)
    else
        console.error("no hdlr set")
}

function elementIsCall(parentAddr: string[], args: any[]) {
    if (root.__fromServer)
        return;
    if (hdlr)
        return hdlr.onJsElementCall(parentAddr, args)
    else
        console.error("no hdlr set")
}

function newObjIsAdded(parentAddr: string[], newObj: any) {
    if (root.__fromServer)
        return;
    if (hdlr)
        return hdlr.onNewObjAdded(parentAddr, newObj)
    else
        console.error("no call cb set")
    return newObj;
}




function isNumeric(str: any) {
    if (typeof str != "string") return false // we only process strings!  
    return !isNaN(str as any) && // use type coercion to parse the _entirety_ of the string (`parseFloat` alone does not do this)...
        !isNaN(parseFloat(str)) // ...and ensure strings of whitespace fail
}

function buildProxy(obj: any, parentAddr: string[]): any {
    console.log("building", obj, parentAddr)
    const subProxies = Object.fromEntries(Object.entries(obj)
        .filter(([k, v]) => typeof (v) == "object")
        .map(([k, v]) => { return [k, buildProxy(v, parentAddr.concat(k as any))] })
    )

    const res = new Proxy(obj, {
        get: (target, k, rcv) => {
            if (typeof (k) == "symbol" || k.startsWith("__"))
                return target[k as string];
            // console.log("firstP get", k, " from ", parentAddr, target[k], typeof (target[k]), target, rcv)

            if (subProxies[k])
                return subProxies[k];


            if (typeof (target[k]) == "function") {
                const serverfunc = target.__getServerFunctionNames ? target.__getServerFunctionNames() : [];
                // console.log("proxyable funcs", serverfunc);
                if (serverfunc.includes(k)) {
                    // console.log("proxying func");
                    return new Proxy(target[k] as any, {
                        apply(target, thisArg: any, argArray: any[]) {
                            // console.log("call Proooooox", target)
                            return elementIsCall(parentAddr.concat(k), argArray)
                            // return target.call(thisArg, argArray);
                        }
                    })
                }
            }

            return target[k];
        },

        set: (target, k, nV, rcv) => {
            const innerJsFunct = Array.isArray(target) && k == "length"
            if (typeof (k) == "symbol" || k.startsWith("__") || innerJsFunct) {
                target[k as string] = nV;
                return true;
            }
            elementIsSet(parentAddr, target, k, nV)
            if (Array.isArray(target) && isNumeric(k) && typeof (nV) == "object" && k) {
                const newO = newObjIsAdded(parentAddr, buildProxy(nV, parentAddr.concat(k)));
                subProxies[k] = newO;
            }
            target[k] = nV;
            return true;
        },

    })
    return res;

}

export function initProxyObj(rootObj: any, _hdlr: JsCliSender) {
    hdlr = _hdlr
    root = rootObj
    return buildProxy(rootObj, [])
}

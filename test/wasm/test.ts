
import { CustomEmbindModule, JSHandler } from "./gen_js/wasm/wasmJsTypes.ts"
import w from "./gen_js/wasm/wasmJs.js"
import * as prox from "./gen_js/wasm/proxyHelpers.ts"
// import * as prox from "../../src/wasm/proxyHelpers.ts"

import { RootAPI_Data } from "./gen_js/decl/RootAPI_Decl.ts";


import * as t from "./tstHelps.ts"

const mod: CustomEmbindModule = await w();


// import WebSocket from "ws"


class HdlRecorder {
    constructor(public hdlr = new mod.JSHandler) {
        hdlr.onSet = this.onSet
        hdlr.onGet = this.onGet
        hdlr.onCall = this.onCall
        hdlr.onCallResp = this.onCallResp
        hdlr.onRootStateSet = this.onRootStateSet
    }
    lastAddr: any; lastVal: any;
    onInit = () => {
    }

    onSet = (arg0: unknown, arg1: unknown) => {
        this.lastAddr = arg0;
        this.lastVal = arg1;
    }

    onGet = (arg0: unknown, arg1: unknown) => {
        this.lastAddr = arg0;
        this.lastVal = arg1;
    }

    onCall = (arg0: string, arg1: string) => {
        this.lastAddr = arg0;
        this.lastVal = arg1;
    }

    onCallResp = (arg0: string, arg1: any) => {
        this.lastAddr = arg0;
        this.lastVal = arg1;
    }

    onRootStateSet = (arg0: string) => {
        this.lastAddr = arg0;

    }

}

const msgHdlrRecorder = new HdlRecorder();

class BinarySenderLogger {
    sendMessage(m: any) {
        console.log("sould send", m)
    }
    delete() { }
}

const binSenderLogger = new BinarySenderLogger();



// }

// t.test("JSON", () => {
//     const obj = {
//         b: BigInt(4)
//     }
//     const obStr = JSON.stringify(obj, t.jsonReplacer)
//     const ob2 = JSON.parse(obStr, t.jsonReviver)
//     console.log(ob2)
//     t.expectDeepEqual(obj, ob2)
//     t.expect((ob2 as any).b === BigInt(4))
// })

// t.test("wasmInst", () => {
//     const l = nex RootAPI_Data()
//     mod.dumpPtr(l)
//     const msgBuilder = new mod.MainBuilder(l)
//     const m = msgBuilder.buildMessageGetState("")
//     console.log(m)
// })

t.test("healthCheck", () => {
    const jsObj = new RootAPI_Data();
    t.expect(typeof (jsObj.__getServerFunctionNames) === "function")

    const rootApiWasmObj = new mod.RootAPI();
    rootApiWasmObj.delete()

})


class BinSenderType {
    lastMessage: any;
    sendMsg(m: any) { }
}

class BinSenderMock {
    lastMessage: any;
    sendMsg(m: any) { this.lastMessage = m; console.log("sending to server", m); }
}
class JsCli {

    constructor(public binSender: BinSenderType) {

    }
    public jsObj = new RootAPI_Data();
    public rootApiWasmObj = new mod.RootAPI();
    public msgBuilder = new mod.MainBuilder(this.rootApiWasmObj)
    public bt = new mod.JsWasmTransport(this.rootApiWasmObj, msgHdlrRecorder, this.jsObj)
    public sender = new prox.JsCliSender(this.msgBuilder, this.bt, this.binSender);
    public proxy = prox.initProxyObj(this.jsObj, this.sender);

    processFromServer(msg: any) {
        const respToServer = this.bt.processMsg(msg);
        this.binSender.lastMessage = respToServer
        if (!!respToServer) {
            this.binSender.sendMsg(respToServer);
        }

    }


    delete() {
        this.rootApiWasmObj.delete()
    }
}

t.test("jsCli", () => {
    const mockSender = new BinSenderMock();
    const jsCli = new JsCli(mockSender);

    // get value from server
    const getStringMsg = jsCli.msgBuilder.buildMessageGet("/string")
    jsCli.processFromServer(getStringMsg)
    t.expect(!!mockSender.lastMessage)
    console.log(mockSender.lastMessage)

    // set value from server
    const setStringMsg = jsCli.msgBuilder.buildMessageSet("/string", "lololooooo")
    jsCli.processFromServer(setStringMsg)
    t.expect(mockSender.lastMessage === undefined)
    t.expect(jsCli.jsObj.string === "lololooooo")


    // set value from js
    jsCli.proxy.string = "lululuuuu"
    t.expect(!!mockSender.lastMessage)

    jsCli.delete();
})

// t.test("proxy", () => {
//     const jsObj = new RootAPI_Data();
//     const rootApiWasmObj = new mod.RootAPI();
//     const msgBuilder = new mod.MainBuilder(rootApiWasmObj)
//     const bt = new mod.JsWasmTransport(rootApiWasmObj, msgHdlrRecorder, jsObj)
//     const sender = new prox.JsCliSender(msgBuilder, bt, binSenderLogger);
//     const proxy = prox.initProxyObj(jsObj, sender)

//     proxy.vec.push(4)
//     t.expect(proxy.vec.length === 1)
//     t.expect(proxy.vec[0] === 4)
//     t.expectDeepEqual(msgHdlrRecorder.lastAddr, "/vec/0")
//     t.expect(proxy.__fromServer === false)
//     mod.dumpPtr(rootApiWasmObj)

//     // proxy.string = "lmlml"
//     // t.expect(proxy.string === "lmlml")
//     // mod.dump();
//     rootApiWasmObj.delete();
// })


// t.test("binTransport", () => {
//     const jsObj = new RootAPI_Data();
//     const rootApiWasmObj = new mod.RootAPI();
//     const msgBuilder = new mod.MainBuilder(rootApiWasmObj)
//     t.expect(typeof (jsObj.__getServerFunctionNames) === "function")
//     const bt = new mod.JsWasmTransport(rootApiWasmObj, msgHdlrRecorder, jsObj)
//     const sender = new prox.JsCliSender(msgBuilder, bt);
//     const obj = prox.initProxyObj(jsObj, sender)
//     obj.vec.push(4)
//     obj.string = "lolo"
//     // printJsObj(obj)

//     // mod.dump();
//     const msg = msgBuilder.buildMessageGetState("")
//     console.log("get root", msg)
//     const snap = t.getObjSnapshot(obj)
//     const resp = bt.processMsg(msg)
//     obj.a = 3333;
//     obj.vec.length = 0
//     obj.string = ""
//     console.log("set root", resp)
//     const resp2 = bt.processMsg(resp)
//     t.expect(resp2 === undefined)
//     t.printJsObj(obj)
//     t.expectDeepEqual(obj, snap)
//     // mod.dump();

//     rootApiWasmObj.delete();
// })





// function testWS() {
//     const obj = new RootAPI_Data()
//     const conHdlr = new mod.ConnHandler();
//     mod.initWebSocket(9000, conHdlr, msgHdlrRecorder, obj)
// }
// testWS()
t.runAll();


import { CustomEmbindModule } from "./gen_js/wasm/wasmJsTypes"

import w from "./gen_js/wasm/wasmJs.js"

// import WebSocket from "ws"

import { RootAPI_Data } from "./gen_js/decl/RootAPI_Decl.ts";


// import * as w from "./assets/wasmJs.wasm"
const mod: CustomEmbindModule = await w();




const msgHdlr = new mod.JSHandler();
//  = {
//     onInit: () => { },
//     onRootStateSet: () => { },
//     onSet: () => { },
//     onGet: () => { },
//     onCall: () => { },
//     onCallResp: () => { },
// }

function printJsObj(obj: any) {
    console.log("obj\n", JSON.stringify(obj, (key: string, o: any) => { if (typeof o == "bigint") return o.toString(); else return o; }, 2))
}
function testBin() {
    const obj = new RootAPI_Data()
    printJsObj(obj)
    const bt = new mod.BinTransport(msgHdlr, obj)
    const mBuilder = mod.getMessageBuilder();
    mod.dump();
    const msg = mBuilder.buildGetRootStateMessage("")
    console.log("get root", msg)
    const resp = bt.processMsg(msg)
    console.log("set root", resp)
    const resp2 = bt.processMsg(resp)
    console.log("set root resp", resp2)
    printJsObj(obj)
    mod.dump();
}
testBin();


// function testWS() {
//     const obj = new RootAPI_Data()
//     const conHdlr = new mod.ConnHandler();
//     mod.initWebSocket(9000, conHdlr, msgHdlr, obj)
// }
// testWS()


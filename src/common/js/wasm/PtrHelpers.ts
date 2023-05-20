import { EmscriptenModule } from "emscripten"
export class OptEncodedPtr {

    valid = false;
    ptr = -1;
}


function getCodedArrayFromWasmRaw(ctx: EmscriptenModule, rawPtr: number): Uint8Array {
    const bgPtr = BigInt(rawPtr);
    const size = Number(BigInt.asUintN(32, bgPtr >> BigInt(32)));
    const ptr = Number(BigInt.asUintN(32, bgPtr));
    console.log("ptr from wasm", size, ptr);
    return ctx.HEAPU8.subarray(ptr, ptr + size)
}



export function getCodedArrayFromWasm(ctx: any, retVal: OptEncodedPtr | number): Uint8Array {
    if (typeof (retVal) === "number")
        return getCodedArrayFromWasmRaw(ctx, retVal)
    // handleOpt
    if (retVal.valid === true) {
        console.log(" valid opt ptr")
        return getCodedArrayFromWasmRaw(ctx, retVal.ptr)
    }
    // if (retVal.valid === false) {
    console.error("non valid opt ptr")
    return new Uint8Array();
    // }

}


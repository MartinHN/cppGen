let timerId;
let socket;

const useBinaryFormat = true;

function tryReConnect(ctx, port, cause) {
    console.log("will reopen websocket cause ", cause);
    if (socket && socket.readyState === WebSocket.OPEN) {
        try {
            socket.close();
        } catch (e) {
            console.error("cant close sock");
        }
    }
    clearTimeout(timerId);
    timerId = setTimeout(function () {
        initSocket(ctx, port);
    }, 5000);

}

export function sendMessage(msg) {
    socket.send(msg);
}


function getCodedArrayFromWasm(ctx, retVal) {
    // handleOpt
    if (retVal.valid === false)
    {
        console.error("non valid opt ptr")
        return new Uint8Array();
    }
    if (retVal.valid === true) {
        console.log(" valid opt ptr")
        return getCodedArrayFromWasm(ctx, retVal.ptr)
    }
    const size = Number(BigInt.asUintN(32, BigInt(retVal) >> BigInt(32)));
    const ptr = Number(BigInt.asUintN(32, retVal));

    console.log("ptr from wasm",size, ptr);
    return ctx.HEAPU8.subarray(ptr, ptr + size)

}
export function initSocket(ctx, port = 9002) {
    console.log('init socket for', ctx)
    // Create WebSocket connection.
    socket = new WebSocket('ws://localhost:' + port);
    socket.binaryType = "arraybuffer"
    // Connection opened
    socket.addEventListener('open', (event) => {
        clearTimeout(timerId);
        console.log("websocket opened");
        console.log(socket.extensions);
        // sendMessage("getState");
        // ctx.dump();
        const binCoded = ctx.buildIntModMessage("intValue", 60);
        //     const binCoded = ctx.getBin();
        if (binCoded.valid) {
            const arr = getCodedArrayFromWasm(ctx, binCoded);
            socket.send(arr);
        }
        else {
            console.error("invalid messageToSend")
        }
    });

    // Listen for messages
    socket.addEventListener('message', (event) => {
        const d = event.data
        if (typeof (d) === "string") {
            console.log('String Message from server ', d);
        }
        else {
            console.log('Bin Message from server ', d);
            let view = new Uint8Array(d);
            const input_ptr = 0;
            ctx.HEAPU8.set(view, input_ptr); // write WASM memory calling the set method for the Uint8Array
            ctx.loadBin(input_ptr, view.length);
            console.log("loaded")
            ctx.dump();

        }
    });

    socket.addEventListener('close', (event) => {
        tryReConnect(ctx, port, "closing");
    });
    socket.addEventListener('error', (event) => {
        tryReConnect(ctx, port, "error");
    });



}

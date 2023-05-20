// import WebSocket from "isomorphic-ws"
// import * as wsm from "../wasm/PtrHelpers"
// let timerId: NodeJS.Timeout;
// let socket: WebSocket;

// const useBinaryFormat = true;

// function tryReConnect(ctx: EmscriptenModule, port: number, cause: any) {
//     console.log("will reopen websocket cause ", cause);
//     if (socket && socket.readyState === WebSocket.OPEN) {
//         try {
//             socket.close();
//         } catch (e) {
//             console.error("cant close sock");
//         }
//     }
//     clearTimeout(timerId);
//     timerId = setTimeout(function () {
//         initSocket(ctx, port);
//     }, 5000);

// }

// export function sendMessage(msg: any) {
//     socket.send(msg);
// }



// export function initSocket(ctx: EmscriptenModule, port = 9002) {
//     console.log('init socket for', ctx)
//     // Create WebSocket connection.
//     socket = new WebSocket('ws://localhost:' + port);
//     socket.binaryType = "arraybuffer"
//     // Connection opened
//     socket.addEventListener('open', (/* e */) => {
//         clearTimeout(timerId);
//         console.log("websocket opened");
//         console.log(socket.extensions);
//         // sendMessage("getState");
//         // ctx.dump();
//         const binCoded = ctx.buildIntModMessage("intValue", 60);
//         //     const binCoded = ctx.getBin();
//         if (binCoded.valid) {
//             const arr = wsm.getCodedArrayFromWasm(ctx, binCoded);
//             socket.send(arr);
//         }
//         else {
//             console.error("invalid messageToSend")
//         }
//     });

//     // Listen for messages
//     socket.addEventListener('message', (event: MessageEvent) => {
//         const d = event.data
//         if (typeof (d) === "string") {
//             console.log('String Message from server ', d);
//         }
//         else {
//             console.log('Bin Message from server ', d);
//             let view = new Uint8Array(d);
//             const input_ptr = 0;
//             ctx.HEAPU8.set(view, input_ptr); // write WASM memory calling the set method for the Uint8Array
//             ctx.loadBin(input_ptr, view.length);
//             console.log("loaded")
//             ctx.dump();

//         }
//     });

//     socket.addEventListener('close', (/* e */) => {
//         tryReConnect(ctx, port, "closing");
//     });
//     socket.addEventListener('error', (/* e */) => {
//         tryReConnect(ctx, port, "error");
//     });



// }

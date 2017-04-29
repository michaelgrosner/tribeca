export function timeout<T>(ms: number, promise: Promise<T>) : Promise<T> {
    return new Promise(function (resolve, reject) {
        promise.then(resolve);
        setTimeout(function () {
            reject(new Error('Timeout after '+ms+' ms')); // (A)
        }, ms);
    });
}

export function delay(ms: number) : Promise<{}> {
    return new Promise(function (resolve, reject) {
        setTimeout(resolve, ms); // (A)
    });
}
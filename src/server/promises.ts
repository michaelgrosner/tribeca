import * as Bluebird from 'bluebird';

global.Promise = Bluebird;

export function timeout<T>(ms: number, promise: Promise<T>): Promise<T> {
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

export type Deferred<T> = {
    promise: Promise<T>,
    resolve: (T) => void,
    reject: (T) => void
}
export function defer<T>(): Deferred<T> {
    let resolve, reject
    const promise = new Promise<T>((res, rej) => {
        resolve = res
        reject = rej
    })
    return {
        promise,
        resolve,
        reject
    }
}
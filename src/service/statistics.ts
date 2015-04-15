export interface IComputeStatistics {
    latest : number;
    initialize(seedData : number[]) : void;
    addNewValue(value : number) : number;
}

export class EwmaStatisticCalculator implements IComputeStatistics {
    constructor(private _alpha : number) {}

    public latest : number = null;

    initialize(seedData: number[]) {
        for (var i = 0; i < seedData.length; i++)
            this.addNewValue(seedData[i]);
    }

    addNewValue(value: number): number {
        this.latest = computeEwma(value, this.latest, this._alpha);
        return this.latest;
    }
}

export function computeEwma(newValue: number, previous: number, alpha: number) : number {
    if (previous !== null) {
        return alpha * newValue + (1 - alpha) * previous;
    }

    return newValue;
}
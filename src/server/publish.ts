import Models = require("../share/models");

export class Publisher {
  private _notepad: string;
  private _toggleConfigs: boolean = true;
  constructor(
    public registerSnapshot,
    public registerReceiver,
    public publish,
  ) {
    this.registerSnapshot(Models.Topics.Notepad, () => [this._notepad]);

    this.registerSnapshot(Models.Topics.ToggleConfigs, () => [this._toggleConfigs]);

    this.registerReceiver(Models.Topics.Notepad, (notepad: string) => {
      this._notepad = notepad;
    });

    this.registerReceiver(Models.Topics.ToggleConfigs, (toggleConfigs: boolean) => {
      this._toggleConfigs = toggleConfigs;
    });
  }
}

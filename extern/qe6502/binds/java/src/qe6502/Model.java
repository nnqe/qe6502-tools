package qe6502;

/** Supported qe6502 CPU models. */
public enum Model {
    /** NMOS 6502 model. */
    NMOS(0),

    /** NES/RP2A03 CPU model. */
    NES(1),

    /** WDC 65C02 model. */
    WDC(2),

    /** Rockwell 65C02 model. */
    ROCKWELL(3),

    /** Synertek 65C02 model. */
    ST(4);

    private final int abiValue;

    Model(int abiValue) {
        this.abiValue = abiValue;
    }

    int abiValue() {
        return abiValue;
    }

    static Model fromAbiValue(int value) {
        for (Model model : values()) {
            if (model.abiValue == value) {
                return model;
            }
        }

        throw new IllegalArgumentException("Invalid qe6502 CPU model: " + value);
    }
}

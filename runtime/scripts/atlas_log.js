
export const Debug = {
    print(message) {
        globalThis.print(message);
    },
    warning(message) {
        globalThis.print(`[WARNING] ${message}`);
    },
    error(message) {
        globalThis.print(`[ERROR] ${message}`);
    }
};
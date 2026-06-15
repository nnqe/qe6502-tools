export const GITHUB_BASE_URLS_BY_MODEL = Object.freeze({
  nmos: "https://raw.githubusercontent.com/SingleStepTests/65x02/main/6502/v1/",
  nes: "https://raw.githubusercontent.com/SingleStepTests/65x02/main/nes6502/v1/",
  wdc: "https://raw.githubusercontent.com/SingleStepTests/65x02/main/wdc65c02/v1/",
  rw: "https://raw.githubusercontent.com/SingleStepTests/65x02/main/rockwell65c02/v1/",
  st: "https://raw.githubusercontent.com/SingleStepTests/65x02/main/synertek65c02/v1/",
});

function hexOpcode(opcode) {
  return (opcode & 0xff).toString(16).padStart(2, "0").toLowerCase();
}

function normalizeBaseUrl(baseUrl) {
  return String(baseUrl).trim().replace(/\/+$/u, "") + "/";
}

function parseOpcodeFromName(name) {
  const match = /(^|\/|\\)([0-9a-fA-F]{2})\.json$/u.exec(name);
  return match === null ? null : Number.parseInt(match[2], 16) & 0xff;
}

function makeLocalFileDescriptor(file) {
  const name = file.webkitRelativePath || file.name;
  const opcode = parseOpcodeFromName(name);

  if (opcode === null) {
    return null;
  }

  return {
    name,
    opcode,
    source: "local",
    loadText: async () => file.text(),
  };
}

export function getGithubBaseUrlForModel(modelName) {
  const url = GITHUB_BASE_URLS_BY_MODEL[modelName];

  if (url === undefined) {
    throw new RangeError(`No GitHub SingleStepTests URL configured for model '${modelName}'`);
  }

  return url;
}

export function sourceFromGithubOpcode(baseUrl, opcode, { allowMissing = false } = {}) {
  const normalizedBaseUrl = normalizeBaseUrl(baseUrl);
  const op = opcode & 0xff;
  const name = `${hexOpcode(op)}.json`;
  const url = normalizedBaseUrl + name;

  return [
    {
      name,
      opcode: op,
      source: "github",
      url,
      loadText: async () => {
        let response;
        try {
          response = await fetch(url, {
            cache: "no-store",
            headers: { Accept: "application/json" },
          });
        } catch (error) {
          const detail = error instanceof Error ? `${error.name}: ${error.message}` : String(error);
          throw new Error(`Network fetch failed for ${url}: ${detail}`);
        }

        if (allowMissing && response.status === 404) {
          return undefined;
        }

        if (!response.ok) {
          throw new Error(`Failed to fetch ${url}: ${response.status} ${response.statusText}`);
        }

        return response.text();
      },
    },
  ];
}

export function sourceFromGithubFull(baseUrl) {
  const descriptors = [];

  for (let opcode = 0; opcode <= 0xff; ++opcode) {
    descriptors.push(...sourceFromGithubOpcode(baseUrl, opcode, { allowMissing: true }));
  }

  return descriptors;
}

export function sourceFromLocalFile(file) {
  const descriptor = makeLocalFileDescriptor(file);

  if (descriptor === null) {
    throw new Error(`Local file is not named like an opcode JSON file: ${file.name}`);
  }

  return [descriptor];
}

export function sourceFromLocalFolder(fileList) {
  const descriptors = Array.from(fileList)
    .map(makeLocalFileDescriptor)
    .filter((descriptor) => descriptor !== null)
    .sort((a, b) => a.opcode - b.opcode || a.name.localeCompare(b.name));

  if (descriptors.length === 0) {
    throw new Error("No opcode JSON files named 00.json..ff.json were found in the selected folder");
  }

  return descriptors;
}

export function parseOpcodeText(text) {
  const trimmed = String(text).trim();

  if (!/^[0-9a-fA-F]{1,2}$/u.test(trimmed)) {
    throw new RangeError(`Opcode must be one or two hex digits, got '${text}'`);
  }

  return Number.parseInt(trimmed, 16) & 0xff;
}

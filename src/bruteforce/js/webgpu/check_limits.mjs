if (typeof navigator === 'undefined' || !navigator.gpu) {
    const { create, globals } = await import('webgpu');
    Object.assign(globalThis, globals);
    Object.defineProperty(globalThis, 'navigator', { value: { gpu: create([]) }, configurable: true });
}
const adapter = await navigator.gpu.requestAdapter();
const device  = await adapter.requestDevice();
console.log('maxComputeWorkgroupsPerDimension:', device.limits.maxComputeWorkgroupsPerDimension);
console.log('maxComputeInvocationsPerWorkgroup:', device.limits.maxComputeInvocationsPerWorkgroup);
console.log('maxBufferSize:', device.limits.maxBufferSize);
const maxCandidates = device.limits.maxComputeWorkgroupsPerDimension * 64;
console.log(`Max candidates per dispatch: ${maxCandidates} (2^${Math.log2(maxCandidates).toFixed(1)})`);
device.destroy();

/* FreeBASIC runtime tests: js-driver.cjs.
 * Purpose: Exercise the production Web Crypto binding and failure boundaries.
 * Responsibilities: Real provider, quota limits, scratch ownership and heap guards.
 * This test does not replace the public C ABI or qualify a physical browser. */
'use strict';
const assert=require('node:assert/strict');
const fs=require('node:fs');
const path=require('node:path');
const vm=require('node:vm');
const {webcrypto}=require('node:crypto');
const source=fs.readFileSync(path.resolve(__dirname,'../../src/rtlib/drivers/js/sys_hsecure_random.c'),'utf8');
const start=source.indexOf('EM_JS(int, js_secure_random,');
assert.ok(start>=0);
const body=source.slice(source.indexOf('{',start)+1,source.indexOf('\n});',start));
function fixture(provider){
 const heap=new Uint8Array(1048576+32).fill(0xa5);
 const context=vm.createContext({crypto:provider,HEAPU8:heap});
 vm.runInContext('function fill(destination,byte_count){'+body+'\n}',context);
 return {heap,fill:context.fill};
}
for(const length of [1,16,65536,65537,1048576]){
 const f=fixture(webcrypto);
 assert.equal(f.fill(16,length),1);
 assert.ok(f.heap.slice(0,16).every(b=>b===0xa5));
 assert.ok(f.heap.slice(16+length).every(b=>b===0xa5));
 if(length>=16)assert.ok(f.heap.slice(16,16+length).some(b=>b!==0xa5));
}
let lengths=[],scratch=[];
const quota=fixture({getRandomValues(buffer){
 assert.ok(buffer.length<=65536);lengths.push(buffer.length);scratch.push(buffer);
 buffer.fill(0x73);return buffer;
}});
assert.equal(quota.fill(16,1048576),1);
assert.equal(lengths.length,16);
assert.equal(lengths.reduce((a,b)=>a+b,0),1048576);
assert.ok(scratch.every(buffer=>buffer.every(b=>b===0)),'scratch is cleared after success');
assert.ok(quota.heap.slice(16,16+1048576).every(b=>b===0x73));
let calls=0;
const failure=fixture({getRandomValues(buffer){calls++;buffer.fill(0x42);scratch=[buffer];throw new Error('provider failure');}});
assert.equal(failure.fill(16,80),-1);
assert.equal(calls,1);
assert.ok(scratch[0].every(b=>b===0),'scratch is cleared after failure');
assert.ok(failure.heap.every(b=>b===0xa5),'a failed first chunk is never copied');
assert.equal(failure.fill(16,failure.heap.length),-1);
assert.equal(failure.fill(-1,16),-1);
assert.equal(calls,1,'invalid heap spans never reach the provider');
const absent=fixture(undefined);
assert.equal(absent.fill(16,16),0);
assert.ok(absent.heap.every(b=>b===0xa5));
console.log('JS_SECURE_RANDOM_DRIVER=PASS');
/* end of js-driver.cjs */

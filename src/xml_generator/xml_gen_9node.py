nnodes = 9
ngpuspernode = 8
instances = 2
nchunksperloop = nnodes*ngpuspernode*instances
print('<algo name="test" nchunksperloop="{}" nchannels="{}" proto="Simple">'.format(nchunksperloop, instances))

def CrossNodeNghr(node, g):
    nghrNode = g if node > g else g+1
    nghrG = node if nghrNode > node else node-1
    return nghrNode, nghrG, nghrNode * ngpuspernode + nghrG
for node in range(nnodes):
    for g in range(ngpuspernode):
        tbindex = 0
        nghrNode, nghrG, crossnodenghr = CrossNodeNghr(node,g)
        print('  <gpu id="{}" i_chunks="{}" o_chunks="{}" s_chunks="{}">'.format(node*ngpuspernode+g, nchunksperloop, nchunksperloop, instances*2*ngpuspernode**2))
        for ch in range(instances):
            print('    <tb id="{}" send="{}" recv="-1" chan="{}">'.format(tbindex, crossnodenghr, ch))
            print('      <step s="0" type="s" srcbuf="s" srcoff="{}" dstbuf="s" dstoff="{}" cnt="{}" depid="{}" deps="{}" hasdep="0"/>'.format(ch*ngpuspernode**2, instances*ngpuspernode**2+ch*ngpuspernode**2, ngpuspernode**2, instances*(2+2*g)+ch, ngpuspernode-1))
            print('    </tb>')
            tbindex+=1
        for ch in range(instances):
            print('    <tb id="{}" send="-1" recv="{}" chan="{}">'.format(tbindex, crossnodenghr, ch))
            print('      <step s="0" type="r" srcbuf="s" srcoff="{}" dstbuf="s" dstoff="{}" cnt="{}" depid="-1" deps="-1" hasdep="1"/>'.format(ch*ngpuspernode**2, instances*ngpuspernode**2+ch*ngpuspernode**2, ngpuspernode**2))
            print('    </tb>')
            tbindex+=1
        for withinnodenghr  in range(ngpuspernode):
            withinNghrNode, withinNghrG, withinCrossNodeNghr = CrossNodeNghr(node, withinnodenghr)
            if withinnodenghr == g:
                for ch in range(instances):
                    step = 0
                    print('    <tb id="{}" send="-1" recv="-1" chan="0">'.format(tbindex))
                    print('      <step s="{}" type="cpy" srcbuf="i" srcoff="{}" dstbuf="s" dstoff="{}" cnt="{}" depid="-1" deps="-1" hasdep="{}"/>'.format(step, instances*nghrNode*ngpuspernode+ch, instances*g*ngpuspernode+ch, ngpuspernode, 1 if step == 1+ngpuspernode-2 else 0))
                    step += 1
                    for j in range(ngpuspernode):
                        if j != g:
                            print('      <step s="{}" type="nop" srcbuf="i" srcoff="0" dstbuf="o" dstoff="0" cnt="0" depid="{}" deps="{}" hasdep="{}"/>'.format(step, (instances*(2*j+2+1)+ch) if j < g else (instances*(2*j+2)+ch), 0, 1 if step == 1+ngpuspernode-2 else 0))
                            step += 1
                    print('      <step s="{}" type="cpy" srcbuf="i" srcoff="{}" dstbuf="o" dstoff="{}" cnt="{}" depid="-1" deps="-1" hasdep="0"/>'.format(step, instances*(node*ngpuspernode+g)+ch, instances*(node*ngpuspernode+g)+ch, 1))
                    step += 1
                    for j in range(ngpuspernode):
                        print('      <step s="{}" type="cpy" srcbuf="s" srcoff="{}" dstbuf="o" dstoff="{}" cnt="{}" depid="{}" deps="{}" hasdep="0"/>'.format(step, instances*(ngpuspernode**2+j*ngpuspernode+g)+ch, instances*(nghrNode*ngpuspernode+j)+ch, 1, instances*1+ch if step == ngpuspernode+1 else -1, 0 if step == ngpuspernode+1 else -1))
                        step += 1
                    print('    </tb>')
                    tbindex+=1
            else:
                for ch in range(instances):
                    print('    <tb id="{}" send="{}" recv="-1" chan="{}">'.format(tbindex, node*ngpuspernode+withinnodenghr, ch))
                    print('      <step s="0" type="s" srcbuf="i" srcoff="{}" dstbuf="s" dstoff="{}" cnt="{}" depid="-1" deps="-1" hasdep="0"/>'.format(instances*withinNghrNode*ngpuspernode+ch, instances*g*ngpuspernode+ch, ngpuspernode))
                    print('      <step s="1" type="s" srcbuf="i" srcoff="{}" dstbuf="o" dstoff="{}" cnt="{}" depid="-1" deps="-1" hasdep="0"/>'.format(instances*(node*ngpuspernode+withinnodenghr)+ch, instances*(node*ngpuspernode+g)+ch, 1))
                    step = 2
                    for j in range(ngpuspernode):
                        print('      <step s="{}" type="s" srcbuf="s" srcoff="{}" dstbuf="o" dstoff="{}" cnt="{}" depid="{}" deps="{}" hasdep="0"/>'.format(step, instances*(ngpuspernode**2+j*ngpuspernode+withinnodenghr)+ch, instances*(nghrNode*ngpuspernode+j)+ch, 1, instances*1+ch if j == 0 else -1, 0 if j == 0 else -1))
                        step += 1
                    print('    </tb>')
                    tbindex+=1
                for ch in range(instances):
                    print('    <tb id="{}" send="-1" recv="{}" chan="{}">'.format(tbindex, node*ngpuspernode+withinnodenghr, ch))
                    print('      <step s="0" type="r" srcbuf="i" srcoff="{}" dstbuf="s" dstoff="{}" cnt="{}" depid="-1" deps="-1" hasdep="1"/>'.format(instances*nghrNode*ngpuspernode+ch, instances*withinnodenghr*ngpuspernode+ch, ngpuspernode))
                    print('      <step s="1" type="r" srcbuf="i" srcoff="{}" dstbuf="o" dstoff="{}" cnt="{}" depid="-1" deps="-1" hasdep="0"/>'.format(instances*(node*ngpuspernode+g)+ch, instances*(node*ngpuspernode+withinnodenghr)+ch, 1))
                    step = 2
                    for j in range(ngpuspernode):
                        print('      <step s="{}" type="r" srcbuf="s" srcoff="{}" dstbuf="o" dstoff="{}" cnt="{}" depid="-1" deps="-1" hasdep="0"/>'.format(step, instances*(ngpuspernode**2+j*ngpuspernode+g)+ch, instances*(withinNghrNode*ngpuspernode+j)+ch, 1))
                        step += 1
                    print('    </tb>')
                    tbindex+=1
        print('  </gpu>')
print('</algo>')

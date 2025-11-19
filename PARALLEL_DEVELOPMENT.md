# Parallel Development Strategy

## Git Worktree Setup

We can accelerate development by running multiple Claude sessions in parallel, each working on different features in separate git worktrees.

### Worktree Structure

```
~/code/PearVisor/              (main worktree - main branch)
~/code/PearVisor-vm/           (feature/vm-lifecycle)
~/code/PearVisor-gpu/          (feature/gpu-passthrough)
~/code/PearVisor-ui/           (feature/ui-enhancements)
~/code/PearVisor-network/      (feature/networking)
```

### Setup Commands

```bash
cd ~/code/PearVisor

# Create worktrees for parallel development
git worktree add ../PearVisor-vm -b feature/vm-lifecycle
git worktree add ../PearVisor-gpu -b feature/gpu-passthrough
git worktree add ../PearVisor-ui -b feature/ui-enhancements
git worktree add ../PearVisor-network -b feature/networking
```

## Parallel Workstreams

### Stream 1: VM Lifecycle (PearVisor-vm/)
**Branch:** `feature/vm-lifecycle`

**Tasks:**
- [ ] Implement VZVirtualMachine configuration
- [ ] Add VZLinuxBootLoader with kernel/initrd
- [ ] Configure VZVirtioBlockDeviceConfiguration
- [ ] Add VZVirtioConsoleDeviceConfiguration
- [ ] Implement start/stop/pause/resume
- [ ] Error handling and state management

**Dependencies:** None
**Estimated Time:** 4-6 hours
**Priority:** CRITICAL (blocks everything else)

---

### Stream 2: GPU Passthrough (PearVisor-gpu/)
**Branch:** `feature/gpu-passthrough`

**Tasks:**
- [ ] Build MoltenVK from submodule
- [ ] Port virglrenderer for macOS
- [ ] Implement virtio-gpu device in C
- [ ] Create Venus protocol handler
- [ ] Build MoltenVK bridge
- [ ] Test with glxgears

**Dependencies:** VM lifecycle must work first
**Estimated Time:** 8-12 hours
**Priority:** HIGH

---

### Stream 3: UI Enhancements (PearVisor-ui/)
**Branch:** `feature/ui-enhancements`

**Tasks:**
- [ ] VM console viewer (VNC/serial display)
- [ ] Real-time performance monitoring
- [ ] GPU metrics dashboard
- [ ] VM screenshot/recording
- [ ] Settings persistence
- [ ] Keyboard shortcuts

**Dependencies:** Can work in parallel
**Estimated Time:** 4-6 hours
**Priority:** MEDIUM

---

### Stream 4: Networking (PearVisor-network/)
**Branch:** `feature/networking`

**Tasks:**
- [ ] VZNATNetworkDeviceAttachment
- [ ] VZBridgedNetworkDeviceAttachment
- [ ] Port forwarding configuration
- [ ] Network traffic monitoring
- [ ] DNS configuration
- [ ] Firewall rules

**Dependencies:** Can work in parallel
**Estimated Time:** 3-4 hours
**Priority:** MEDIUM

---

## Workflow

### For Each Claude Session:

1. **Open in separate window/tab:**
   ```bash
   cd ~/code/PearVisor-{stream}
   code . # or your editor
   ```

2. **Start Claude Code in that directory:**
   - Each Claude instance works independently
   - Each has its own branch
   - No merge conflicts during development

3. **Development cycle:**
   - Make changes in worktree
   - Commit locally: `git commit -m "feat: ..."`
   - Push branch: `git push -u origin feature/...`

4. **Integration:**
   - Create PR on GitHub
   - Review and merge to main
   - Other worktrees rebase on main

### Coordination

**Main worktree (~/code/PearVisor):**
- Integration point
- Run tests on merged features
- Tag releases
- Update documentation

**Communication:**
- Each Claude session documents progress in branch
- STATUS.md updated per feature
- PRs reviewed before merge

---

## Benefits

### Speed
- 4x parallelization = 4x faster development
- Independent features don't block each other
- Can work on UI while VM backend is building

### Isolation
- Each feature has clean working directory
- No accidental cross-contamination
- Easy to abandon failed experiments

### Git Advantages
- Clean feature branches
- Easy to review PRs
- Can cherry-pick commits
- Revert features without affecting others

---

## Example Usage

### Session 1 (Critical Path):
```bash
cd ~/code/PearVisor-vm
# Claude implements VM lifecycle
# This is blocking, so do first
```

### Session 2 (GPU - Can start after VM boots):
```bash
cd ~/code/PearVisor-gpu
# Claude implements GPU passthrough
# Waits for VM lifecycle PR to merge
```

### Session 3 (Parallel - Independent):
```bash
cd ~/code/PearVisor-ui
# Claude enhances UI
# Can work immediately, doesn't need VM working
```

### Session 4 (Parallel - Independent):
```bash
cd ~/code/PearVisor-network
# Claude implements networking
# Can work immediately
```

---

## Integration Strategy

### Phase 1: Foundation (Serial)
1. Merge `feature/vm-lifecycle` first (MUST WORK)
2. Test: Can we boot Ubuntu?
3. If yes, proceed to Phase 2

### Phase 2: Parallel Features
1. Start all 3 remaining streams simultaneously
2. `feature/gpu-passthrough` (depends on Phase 1)
3. `feature/ui-enhancements` (independent)
4. `feature/networking` (independent)

### Phase 3: Integration
1. Merge all features to main
2. Test combined functionality
3. Fix integration issues
4. Tag v0.1.0-alpha

---

## Conflict Resolution

### If worktrees diverge:
```bash
# In worktree that needs update
git fetch origin
git rebase origin/main

# If conflicts
git status
# Fix conflicts
git add .
git rebase --continue
```

### If feature is abandoned:
```bash
# Remove worktree
cd ~/code
git worktree remove PearVisor-{stream}
git branch -D feature/{stream}
```

---

## Ready to Launch?

**Command to set up all worktrees:**
```bash
cd ~/code/PearVisor
git worktree add ../PearVisor-vm -b feature/vm-lifecycle
git worktree add ../PearVisor-gpu -b feature/gpu-passthrough
git worktree add ../PearVisor-ui -b feature/ui-enhancements
git worktree add ../PearVisor-network -b feature/networking
```

**Then open 4 Claude Code sessions:**
1. Terminal 1: `cd ~/code/PearVisor-vm && claude-code`
2. Terminal 2: `cd ~/code/PearVisor-gpu && claude-code`
3. Terminal 3: `cd ~/code/PearVisor-ui && claude-code`
4. Terminal 4: `cd ~/code/PearVisor-network && claude-code`

**Assign tasks:**
- Claude 1: "Implement VM lifecycle (CRITICAL PATH)"
- Claude 2: "Implement GPU passthrough (wait for Claude 1)"
- Claude 3: "Enhance UI with console viewer"
- Claude 4: "Implement networking stack"

**LET'S GO! 🚀**

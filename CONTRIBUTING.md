# Contributing to PearVisor

Thank you for your interest in contributing to PearVisor! This document provides guidelines and information for contributors.

## Code of Conduct

We follow the [Contributor Covenant Code of Conduct](https://www.contributor-covenant.org/version/2/1/code_of_conduct/). Be respectful, inclusive, and constructive.

## Getting Started

### Prerequisites

- macOS 15+ (Sequoia)
- Xcode 15+
- CMake 3.25+
- Git with submodules support

### Setting Up Development Environment

1. **Fork and clone the repository:**
   ```bash
   git clone https://github.com/yourusername/pearvisor.git
   cd pearvisor
   ```

2. **Initialize submodules:**
   ```bash
   git submodule update --init --recursive
   ```

3. **Build the GPU subsystem:**
   ```bash
   cd GPU
   mkdir build && cd build
   cmake ..
   make -j$(sysctl -n hw.ncpu)
   ```

4. **Open in Xcode:**
   ```bash
   cd ../..
   open Package.swift
   ```

## Project Structure

```
pearvisor/
├── Sources/           # Swift source code
│   ├── PearVisor/     # Main GUI application
│   ├── PearVisorCore/ # Core VM management library
│   └── PearVisorGPU/  # Swift wrapper for GPU subsystem
├── GPU/               # C/C++ GPU subsystem
│   ├── include/       # Public C headers
│   └── src/           # Implementation
├── Submodules/        # External dependencies (MoltenVK, virglrenderer)
├── Tests/             # Unit and integration tests
└── Documentation/     # Extended documentation
```

## Development Workflow

### Branching Strategy

- `main`: Stable, production-ready code
- `develop`: Integration branch for features
- `feature/*`: Feature branches (e.g., `feature/gpu-passthrough`)
- `bugfix/*`: Bug fix branches
- `release/*`: Release preparation branches

### Making Changes

1. **Create a feature branch:**
   ```bash
   git checkout -b feature/my-feature
   ```

2. **Make your changes:**
   - Write clean, documented code
   - Follow existing code style (see below)
   - Add tests for new functionality
   - Update documentation if needed

3. **Test your changes:**
   ```bash
   # Run Swift tests
   swift test
   
   # Build GPU subsystem
   cd GPU/build && make
   ```

4. **Commit your changes:**
   ```bash
   git add .
   git commit -m "feat: add GPU memory management"
   ```

5. **Push and create a pull request:**
   ```bash
   git push origin feature/my-feature
   ```

### Commit Message Convention

We use [Conventional Commits](https://www.conventionalcommits.org/):

```
<type>(<scope>): <description>

[optional body]

[optional footer]
```

**Types:**
- `feat`: New feature
- `fix`: Bug fix
- `docs`: Documentation changes
- `style`: Code style changes (formatting, no logic change)
- `refactor`: Code refactoring
- `test`: Adding or updating tests
- `chore`: Maintenance tasks (dependencies, build config)
- `perf`: Performance improvements

**Examples:**
```
feat(gpu): implement Venus protocol handler
fix(vm): resolve memory leak in VM lifecycle
docs(readme): update installation instructions
refactor(core): simplify VM configuration API
```

## Code Style

### Swift

- Follow [Swift API Design Guidelines](https://swift.org/documentation/api-design-guidelines/)
- Use SwiftFormat (config included in repo)
- Use SwiftLint (config included in repo)
- 4 spaces for indentation
- Max line length: 120 characters
- Use `// MARK: -` for section organization

**Example:**
```swift
public class VirtualMachine: ObservableObject {
    // MARK: - Properties
    
    public let id: UUID
    @Published public private(set) var state: VMState
    
    // MARK: - Initialization
    
    public init(id: UUID, configuration: VMConfiguration) {
        self.id = id
        self.configuration = configuration
    }
    
    // MARK: - VM Lifecycle
    
    public func start() async throws {
        // Implementation
    }
}
```

### C/C++

- Follow [Google C++ Style Guide](https://google.github.io/styleguide/cppguide.html) (with exceptions)
- 4 spaces for indentation
- Max line length: 100 characters
- Use `// MARK: -` for section organization
- Document public APIs with Doxygen-style comments

**Example:**
```c
/**
 * Initialize the GPU subsystem
 * @return PV_GPU_OK on success, error code otherwise
 */
pv_gpu_error_t pv_gpu_init(void) {
    // Implementation
}
```

## Testing

### Unit Tests

- Write tests for all new functionality
- Aim for 80%+ code coverage
- Use XCTest for Swift code
- Place tests in `Tests/` directory

**Example:**
```swift
import XCTest
@testable import PearVisorCore

final class VirtualMachineTests: XCTestCase {
    func testVMCreation() throws {
        let config = VMConfiguration(name: "Test VM", guestOS: "Ubuntu")
        let vm = VirtualMachine(configuration: config)
        XCTAssertEqual(vm.name, "Test VM")
    }
}
```

### Integration Tests

- Test VM lifecycle (create, start, stop)
- Test GPU passthrough functionality
- Test networking and storage

### Performance Testing

- Benchmark GPU performance vs. native Metal
- Profile CPU and memory usage
- Track performance regressions

## Documentation

### Code Documentation

- Document all public APIs
- Use Swift-style documentation comments (///)
- Include usage examples for complex APIs

**Example:**
```swift
/// Starts the virtual machine
///
/// This method asynchronously starts the VM and transitions it to the running state.
/// The VM must be in a stopped state before calling this method.
///
/// - Throws: `VMError.invalidState` if the VM is not stopped
/// - Throws: `VMError.startupFailed` if the VM fails to start
///
/// Example:
/// ```swift
/// try await vm.start()
/// ```
public func start() async throws {
    // Implementation
}
```

### Extended Documentation

- Add detailed guides to `Documentation/`
- Update README.md for user-facing changes
- Update DESIGN.md for architectural changes

## Pull Request Process

1. **Ensure your PR:**
   - Has a clear title and description
   - References related issues (e.g., "Fixes #123")
   - Includes tests for new functionality
   - Updates documentation if needed
   - Passes all CI checks

2. **PR Review:**
   - At least one maintainer approval required
   - Address review feedback promptly
   - Keep PRs focused and reasonably sized

3. **Merging:**
   - Maintainers will merge approved PRs
   - PRs may be squashed or rebased

## Issue Reporting

### Bug Reports

Use the bug report template and include:
- macOS version and hardware
- PearVisor version
- Steps to reproduce
- Expected vs. actual behavior
- Logs/screenshots if applicable

### Feature Requests

Use the feature request template and include:
- Clear description of the feature
- Use case and motivation
- Proposed implementation (if any)
- Willingness to contribute

## Areas for Contribution

We especially need help with:

### High Priority
- **GPU Passthrough:** virtio-gpu, virglrenderer, MoltenVK integration
- **Performance Optimization:** Reduce Vulkan→Metal overhead
- **Testing:** Comprehensive test coverage

### Medium Priority
- **Networking:** Bridged networking, port forwarding
- **Storage:** Disk image management, snapshots
- **FEX Integration:** x86_64 emulation support

### Low Priority (Future)
- **Windows Support:** ARM64 Windows guests
- **Container Support:** GPU-accelerated containers
- **Documentation:** Tutorials, architecture guides

## Communication

- **GitHub Discussions:** Feature requests, Q&A, design discussions
- **Discord:** Real-time chat (coming soon)
- **Issues:** Bug reports, feature requests

## Recognition

Contributors will be:
- Listed in CONTRIBUTORS.md
- Mentioned in release notes
- Credited in the about dialog

## Questions?

If you have questions, please:
1. Check existing documentation
2. Search GitHub Issues and Discussions
3. Ask in Discord (coming soon)
4. Open a GitHub Discussion

---

Thank you for contributing to PearVisor! 🎉

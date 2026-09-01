# Contributing to Eunoia Engine

Thank you for your interest in contributing to Eunoia Engine!

## Workflow

1. Fork the repository and create a feature branch:
   ```bash
   git checkout -b feat/my-feature
   ```
2. Install dependencies:
   ```bash
   npm install
   ```
3. Implement your feature or bug fix across the relevant `plugins/` or `apps/` directory.
4. Verify TypeScript compilation:
   ```bash
   npx tsc --noEmit
   ```
5. Submit a descriptive Pull Request targeting the `main` branch.

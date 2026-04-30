# AI Agent Operating Guidelines

This file contains global instructions for AI coding sessions in this project.

## Workflow Requirement: "Modify -> Review" (修改->審查)

For every new session and every task:
1. **Modification Phase:** Complete the required code modifications as requested by the user.
2. **Review Phase:** ALWAYS conduct a rigorous self code review after the modifications are complete. Do not wait for the user to ask for a code review.
3. **Reporting:** Present the findings of the code review directly to the user. Look for memory leaks, inefficient algorithms, bad practices (like `using namespace std;` in headers), poor error handling, uninitialized variables, and edge cases.
4. **Iterate:** If the review finds issues, proactively ask the user if you should fix them or apply the fixes directly depending on the severity.

*This workflow is mandatory for all future modifications in this workspace.*

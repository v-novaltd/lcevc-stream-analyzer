<!--
Copyright (C) 2014-2025 V-Nova International Limited

    * All rights reserved.
    * This software is licensed under the BSD-3-Clause-Clear License.
    * No patent licenses are granted under this license. For enquiries about patent
      licenses, please contact legal@v-nova.com.
    * The software is a stand-alone project and is NOT A CONTRIBUTION to any other
      project.
    * If the software is incorporated into another project, THE TERMS OF THE
      BSD-3-CLAUSE-CLEAR LICENSE AND THE ADDITIONAL LICENSING INFORMATION CONTAINED
      IN THIS FILE MUST BE MAINTAINED, AND THE SOFTWARE DOES NOT AND MUST NOT ADOPT
      THE LICENSE OF THE INCORPORATING PROJECT. However, the software may be
      incorporated into a project under a compatible license provided the
      requirements of the BSD-3-Clause-Clear license are respected, and V-Nova
      International Limited remains licensor of the software ONLY UNDER the
      BSD-3-Clause-Clear license (not the compatible license).

ANY ONWARD DISTRIBUTION, WHETHER STAND-ALONE OR AS PART OF ANY OTHER PROJECT,
REMAINS SUBJECT TO THE EXCLUSION OF PATENT LICENSES PROVISION OF THE
BSD-3-CLAUSE-CLEAR LICENSE.
-->
# Contributing

This document outlines the process for making contributions to the LCEVC Stream Analyzer repository. The objective is to provide a clear guideline that increases development efficiency.

---

### Before Contributing

Before we can incorporate your contributions into the LCEVC Stream Analyzer codebase, you must sign the LCEVC Contributor License Agreement (CLA). This agreement ensures that while you retain ownership of your contributions, you grant us the necessary rights to use, modify, and distribute them as part of the project.

Before submitting a PR please reach out to legal@v-nova.com to sign the [CLA](https://gist.github.com/v-nova-ci/f66ee698bdfda7b1ec9649cc806826a4). Here's a plain-language summary of what you're agreeing to:
- You keep the copyright in your contributions, but you give V-Nova the right to use, modify, and distribute them under the project’s BSD-3-Clause Clear license.
- You confirm the work is yours (or that you have the right to contribute it) and that your contribution doesn’t knowingly infringe anyone else’s IP.
- You grant V-Nova a license to any patents that your contribution reads on, so we can use it freely — including in commercial and sublicensed contexts.¹
- You're not required to support your code, and we don’t provide any warranties — just like most open source projects.

This is only a summary — please read the full agreement before signing!

Although you are not required to sign the CLA immediately, we cannot merge your contribution into the project until it has been signed. If you are planning a larger contribution, it is a good idea to raise an issue first to discuss your ideas. This helps us guide you and ensures a smoother contribution process.

¹ Note: The patent license is granted to V-Nova only. The BSD-3-Clause Clear license does not automatically pass on patent rights to downstream users.

---

### Requesting Features

To request a new feature, use the GitHub issue tracker and select the "Feature Request" template at https://github.com/v-novaltd/lcevc-stream-analyzer/issues/new?template=feature_request.yml. Please write your feature requests as a **User Story** and include the following information:

- A clear description of the motivation: why the feature is required and who the user/audience is.
- A clear description of the feature: what it needs to do.
- Information on how the feature can be tested, including any relevant test data.
- Any external dependencies.

We will make every effort to schedule time to develop feature requests as part of the roadmap, but please note that this process may take time.

---

### Reporting Defects

To report a defect, use this project’s GitHub issue tracker and select the "Defect Report" template at https://github.com/v-novaltd/lcevc-stream-analyzer/issues/new?template=bug_report.yml. You will be able to track the progress of the defect resolution via the issue tracker.

Regardless of the nature of your defect report (whether it's a comment or a failure of a complex scenario), please follow these guidelines:

- **Summary:** Provide a high-level description of the defect. For example:
 - *"Playing an MP4 crashes the Android Playground App"*

- **Description:** Include clear and concise details of the defect, along with simple steps to reproduce it. For example:
 - *When selecting an MP4 input file in the attached Android App and hitting play, the application crashes.*
 - *Observed on build date: <10/05/2019, Git Hash: abc123xyz>. Version information is important!*
 - *Android version xxx / API Build.*
 - *Issue only occurs on a Samsung Galaxy S9, not on five other test devices.*
 - *Several different MP4 files were tried, all causing the issue.*

- **Supporting Files:** Provide any relevant debug logs, test content, simple/minimal test apps or configuration files to help reproduce the issue.

The more detailed your bug report, the faster it can be resolved.

---

### Code Contribution

This section outlines the high-level process for contributing features or defect resolutions to the LCEVC Stream Analyzer:

- Source code and documentation changes should be submitted as **Pull Requests** against V-Nova repositories and will be reviewed according to V-Nova’s [coding standards](https://github.com/v-novaltd/coding-standard).
- Ensure your change is associated with a GitHub Issue, and state in the Issue if you intend to or have already developed this feature or defect fix.
- **Engage with V-Nova engineers** about any proposed solutions to defects or feature requests before starting development work. This helps ensure that your contributions align with the project’s goals.

---

### Branching Guidelines

- All work must be performed on a **separate branch** based on **main**, using a public fork.
- Name your branch clearly for easy identification. The format should be `{GitHub Issue number}/{feature_name}` For example:
 - `123/test_improvements`. Use snake case for the feature description with no more than 5 words.

- Ideally, the branch should contain only changes related to the corresponding GitHub Issue. If you need to make additional changes unrelated to the issue, raise a new GitHub Issue and Pull Request.
- Keep branches up to date before raising a Pull Request against main.

---

### Pull Request Guidelines

- Ensure your code follows V-Nova’s [coding standards](https://github.com/v-novaltd/coding-standard).
- For new features, ensure your code coverage is at least 80% of the lines added in the Pull Request.
- Limit pull requests to a single feature or defect fix at a time unless instructed otherwise by a V-Nova reviewer.
- If submitting multiple pull requests for the same feature, prefix the PR title with `[segment #/#]`, where the first number is the segment number and the second is the total number of segments.
- Avoid duplicating effort; if multiple engineers are working on the same Issue, consider collaborating on the Pull Request.
- Add sufficient documentation to ensure your code is well-understood and can be easily used by others.
- Refer to the associated GitHub Issue number in the commit messages and Pull Request title/description.

---

### Style Guidelines

- Use `clang-format` to format your code where applicable.
- Use `clang-tidy` to check the static analysis of your code.
- For CMake files, use `cmake-format`.
- For Python files, use `black` for formatting and `flake8` for static analysis.

---

### Commit Messages

The ideal commit message is concise while being self-explanatory and consistent with existing commit messages. Include:
- The component/module being worked on (e.g., `sdk`).
- A brief description of what the change does.

In most cases, try to reference a GitHub Issue number in the commit message.

---

### Code Review Guidelines

All pull requests must be reviewed by at least one reviewer. If time is short, it's acceptable to ask for a high-level review to validate the approach, and then a subsequent review to look in more detail.

#### Pull Request Checklist

- Branch naming follows `{GitHub Issue number}/{feature_name}`.
- Appropriate code formatting applied.
- Appropriate static analysis performed.
- Appropriate unit tests included.
- Documentation added or updated as necessary.
- All cross-platform issues addressed.
- Appropriate error handling.
- Add context on the change, including how it was tested.

---

### Code of Conduct

To maintain a respectful and productive environment, remember:

- Be patient, polite, understanding and empathetic in all interactions.
- Some contributors may be less experienced than you; please act considerately and remember that everyone is trying to help and improve the repository.
- Take pride in having your code reviewed; reviews provide a valuable opportunity to improve the project. Do not take code review comments personally.

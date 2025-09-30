
document.addEventListener('DOMContentLoaded', function() {
  console.log("DOM fully loaded and parsed (Vanilla JS)");
const themeToggle = document.getElementById('theme-toggle');
const body = document.body;

const prefersDarkColorScheme = window.matchMedia('(prefers-color-scheme: dark)').matches;

if (prefersDarkColorScheme) {
  // User prefers dark mode
  console.log("Dark mode is preferred.");
} else {
  // User prefers light mode or has no preference
  console.log("Light mode is preferred or no preference set.");
}

// Check for saved theme preference in local storage
let savedTheme = localStorage.getItem('theme');
console.log("savedTheme: ", savedTheme);
if(!savedTheme) {
    if(prefersDarkColorScheme) {
        localStorage.setItem('theme', 'dark');
    }
    else {
        localStorage.setItem('theme', 'light');
    }
}

savedTheme = localStorage.getItem('theme');

if (savedTheme === 'dark') {
    body.classList.add('dark-mode');
}
else {
    body.classList.add('light-mode');
}

themeToggle.addEventListener('click', () => {
    let savedTheme = localStorage.getItem('theme');
    console.log("savedTheme: ", savedTheme);
    themeToggle.classList.toggle('dark-mode');
    themeToggle.classList.toggle('light-mode');
    body.classList.toggle('dark-mode');
    body.classList.toggle('light-mode');

    // Save theme preference to local storage
    if (body.classList.contains('dark-mode')) {
        console.log("Switching to dark theme");
        localStorage.setItem('theme', 'dark');
    } else {
        console.log("Switching to ligth theme");
        localStorage.setItem('theme', 'light');
    }
});

// function createTableFromJson(data, tableId) {
//         const table = document.getElementById(tableId);
//         const thead = table.querySelector('thead tr');
//         const tbody = table.querySelector('tbody');

//         // Clear existing content (if any)
//         thead.innerHTML = '';
//         tbody.innerHTML = '';

//         if (data.length === 0) {
//             return; // No data to display
//         }

//         // Create table headers
//         const headers = Object.keys(data[0]);
//         headers.forEach(headerText => {
//             const th = document.createElement('th');
//             th.textContent = headerText;
//             thead.appendChild(th);
//         });

//         // Populate table rows
//         data.forEach(item => {
//             const tr = document.createElement('tr');
//             headers.forEach(header => {
//                 const td = document.createElement('td');
//                 td.textContent = item[header];
//                 tr.appendChild(td);
//             });
//             tbody.appendChild(tr);
//         });
//     }

//     // Call the function to create the table
//     document.addEventListener('DOMContentLoaded', () => {
//         createTableFromJson(jsonData, 'dataTable');
//     });

});
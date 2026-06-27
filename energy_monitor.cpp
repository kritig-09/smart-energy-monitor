#include <iostream>
#include <vector>
#include <thread>
#include <mutex>
#include <fstream>
#include <iomanip>
using namespace std;

mutex mtx; 

class Appliance {
public:
    string name;
    string category;
    float powerRating;
    float dailyHours;
    float lastMonthUnits;
    float monthlyUnitsCached;
    float monthlyCostCached;
    float carbonCached;

    Appliance(string n, string c, float p, float d, float l = 0) {
        name = n; category = c;
        powerRating = p; dailyHours = d;
        lastMonthUnits = l;
        monthlyUnitsCached = 0;
        monthlyCostCached = 0;
        carbonCached = 0;
    }

    void calculate(float rate) {
        monthlyUnitsCached = (powerRating * dailyHours * 30) / 1000;
        monthlyCostCached = monthlyUnitsCached * rate;
        carbonCached = monthlyUnitsCached * 0.82;
    }
};

class EnergyMonitor {
public:
    vector<Appliance> appliances;
    float ratePerUnit;
    float budget;

    EnergyMonitor() { ratePerUnit = 6.0; budget = 0; }

    void calculateAll() {
        vector<thread> threads;
        for (auto& a : appliances) {
            threads.push_back(thread([&a, this]() {
                a.calculate(ratePerUnit);
                lock_guard<mutex> lock(mtx);
                cout << "  [Thread] Calculated: " << a.name << "\n";
            }));
        }
        for (auto& t : threads) t.join();
        cout << "All calculations done (parallel threads).\n";
    }

    void addAppliance() {
        string name, category;
        float power, hours, last;
        cin.ignore();
        cout << "\nAppliance Name: "; getline(cin, name);
        cout << "Category (AC/Lighting/Kitchen/Other): "; getline(cin, category);
        cout << "Power Rating (Watts): "; cin >> power;
        cout << "Daily Usage (Hours): "; cin >> hours;
        cout << "Last Month Units (0 if new): "; cin >> last;
        appliances.push_back(Appliance(name, category, power, hours, last));
        cout << "Added!\n";
    }

    void viewAppliances() {
        if (appliances.empty()) { cout << "\nNo appliances.\n"; return; }
        calculateAll();
        cout << "\n" << left << setw(15) << "Name" << setw(12) << "Category"
             << setw(8) << "Watts" << setw(10) << "Hrs/Day"
             << setw(12) << "Units/Mo" << setw(12) << "Cost/Mo" << "\n";
        cout << string(69, '-') << "\n";
        for (auto& a : appliances) {
            cout << left << setw(15) << a.name << setw(12) << a.category
                 << setw(8) << a.powerRating << setw(10) << a.dailyHours
                 << setw(12) << fixed << setprecision(2) << a.monthlyUnitsCached
                 << "Rs " << a.monthlyCostCached << "\n";
        }
    }

    void calculateBill() {
        calculateAll();
        float total = 0;
        for (auto& a : appliances) total += a.monthlyCostCached;
        cout << "\nTotal Monthly Bill: Rs " << fixed << setprecision(2) << total << "\n";
        if (budget > 0 && total > budget)
            cout << "WARNING: Exceeds budget of Rs " << budget << " by Rs " << total - budget << "\n";
    }

    void peakLoad() {
        if (appliances.empty()) { cout << "\nNo appliances.\n"; return; }
        calculateAll();
        Appliance* peak = &appliances[0];
        for (auto& a : appliances)
            if (a.monthlyCostCached > peak->monthlyCostCached) peak = &a;
        cout << "\nPeak Load: " << peak->name << " | Rs "
             << fixed << setprecision(2) << peak->monthlyCostCached << "/month\n";
    }

    void carbonFootprint() {
        calculateAll();
        float total = 0;
        for (auto& a : appliances) total += a.carbonCached;
        cout << "\n--- Carbon Footprint ---\n";
        for (auto& a : appliances)
            cout << a.name << ": " << fixed << setprecision(2) << a.carbonCached << " kg CO2/month\n";
        cout << "Total: " << total << " kg CO2/month\n";
    }

    void compareLastMonth() {
        calculateAll();
        cout << "\n" << left << setw(15) << "Appliance" << setw(15) << "This Month"
             << setw(15) << "Last Month" << "Difference\n";
        cout << string(55, '-') << "\n";
        for (auto& a : appliances) {
            float diff = a.monthlyUnitsCached - a.lastMonthUnits;
            cout << left << setw(15) << a.name
                 << setw(15) << fixed << setprecision(2) << a.monthlyUnitsCached
                 << setw(15) << a.lastMonthUnits
                 << (diff >= 0 ? "+" : "") << diff << " kWh\n";
        }
    }

    void setBudget() {
        cout << "\nEnter Monthly Budget (Rs): "; cin >> budget;
        cout << "Budget set!\n";
    }

    void saveData() {
        ofstream file("energy_data.txt");
        for (auto& a : appliances)
            file << a.name << "," << a.category << ","
                 << a.powerRating << "," << a.dailyHours << "," << a.lastMonthUnits << "\n";
        file.close();
        cout << "Data saved!\n";
    }

    void loadData() {
        ifstream file("energy_data.txt");
        if (!file) return;
        string line;
        while (getline(file, line)) {
            int c1 = line.find(',');
            int c2 = line.find(',', c1+1);
            int c3 = line.find(',', c2+1);
            int c4 = line.find(',', c3+1);
            string name = line.substr(0, c1);
            string cat = line.substr(c1+1, c2-c1-1);
            float p = stof(line.substr(c2+1, c3-c2-1));
            float d = stof(line.substr(c3+1, c4-c3-1));
            float l = stof(line.substr(c4+1));
            appliances.push_back(Appliance(name, cat, p, d, l));
        }
        file.close();
    }
};

int main() {
    EnergyMonitor monitor;
    monitor.loadData();

    cout << "=== Multithreaded Smart Home Energy Monitor ===\n";
    cout << "Rate per unit set to Rs 6. Change? (1=Yes, 0=No): ";
    int ch; cin >> ch;
    if (ch == 1) { cout << "Enter rate: "; cin >> monitor.ratePerUnit; }

    int choice;
    do {
        cout << "\n--- MENU ---\n";
        cout << "1. Add Appliance\n2. View All Appliances\n3. Calculate Monthly Bill\n";
        cout << "4. Peak Load\n5. Carbon Footprint\n6. Compare Last Month\n";
        cout << "7. Set Budget Alert\n8. Save Data\n9. Exit\n";
        cout << "Enter choice: "; cin >> choice;

        switch(choice) {
            case 1: monitor.addAppliance(); break;
            case 2: monitor.viewAppliances(); break;
            case 3: monitor.calculateBill(); break;
            case 4: monitor.peakLoad(); break;
            case 5: monitor.carbonFootprint(); break;
            case 6: monitor.compareLastMonth(); break;
            case 7: monitor.setBudget(); break;
            case 8: monitor.saveData(); break;
            case 9: cout << "Goodbye!\n"; break;
            default: cout << "Invalid.\n";
        }
    } while(choice != 9);

    return 0;
}
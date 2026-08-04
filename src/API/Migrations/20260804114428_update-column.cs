using Microsoft.EntityFrameworkCore.Migrations;

#nullable disable

namespace API.Migrations
{
    /// <inheritdoc />
    public partial class updatecolumn : Migration
    {
        /// <inheritdoc />
        protected override void Up(MigrationBuilder migrationBuilder)
        {
            migrationBuilder.RenameColumn(
                name: "Type",
                table: "Sensors",
                newName: "Power");

            migrationBuilder.RenameColumn(
                name: "Name",
                table: "Sensors",
                newName: "Equipment");

            migrationBuilder.AddColumn<string>(
                name: "Current",
                table: "Sensors",
                type: "TEXT",
                nullable: false,
                defaultValue: "");
        }

        /// <inheritdoc />
        protected override void Down(MigrationBuilder migrationBuilder)
        {
            migrationBuilder.DropColumn(
                name: "Current",
                table: "Sensors");

            migrationBuilder.RenameColumn(
                name: "Power",
                table: "Sensors",
                newName: "Type");

            migrationBuilder.RenameColumn(
                name: "Equipment",
                table: "Sensors",
                newName: "Name");
        }
    }
}
